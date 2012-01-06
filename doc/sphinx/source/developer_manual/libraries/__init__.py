import subprocess
import logging
import string
import os
import re
from ... import call

#
# Some constants and abbreviations
#
_LOG = logging.getLogger('developer_manual')
_PWD = os.path.abspath(os.path.dirname(__file__))
_TRUNK = os.path.abspath(os.path.join(_PWD, '..', '..', '..', '..', '..'))
_XML = os.path.join(_TRUNK, 'doc', 'sphinx', '_temp', 'xml')
_RST = os.path.join(_TRUNK, 'doc', 'sphinx', '_temp', 'rst', 'developer_manual')
_LIBRARIES = {
  'atermpp':   'ATerm++',    'core':      'Core',     'bes':       'BES',
  'data':      'Data',       'lps':       'LPS',      'lts':       'LTS',
  'pbes':      'PBES',       'process':   'Process',  'trace':     'Trace',
  'utilities': 'Utilities'
}
_DOXYTEMPLATE = string.Template(
  open(os.path.join(_PWD, 'doxy.template')).read())
_REFINDEXTEMPLATE = string.Template(
  open(os.path.join(_PWD, 'reference.rst.template')).read())
_ARTICLEINDEXTEMPLATE= string.Template(
  open(os.path.join(_PWD, 'articles.rst.template')).read())
#
# Generation functions
#
def doxygen(**kwargs):
  '''Run Doxygen. The configuration used is generated by applying the keyword
  arguments to this function to _DOXYTEMPLATE.'''
  call('Doxygen', ['doxygen', '-'], _DOXYTEMPLATE.substitute(kwargs))

def xsltproc(src, transform, dst, xmldir):
  rst = call('xsltproc', 
       ['xsltproc', '--param', 'dir', xmldir, transform, src])
  open(dst, 'w+').write(rst)

def makepdf(src):
  title = re.search(r'\\title{(.*?)}', open(src + '.tex').read(), re.DOTALL)
  title = title.group(1) if title else os.path.basename(src)
  title = ' '.join(title.splitlines())
  if '{' in title or '\\' in title:
    title = os.path.basename(src)
  call('pdflatex', ['pdflatex', src])
  try:
    call('bibtex', ['bibtex', src])
    call('pdflatex', ['pdflatex', src])
    call('pdflatex', ['pdflatex', src])
  except RuntimeError:
    pass
  return title

def generate_library_xml(lib_dir, lib_name):
  '''Generate Doxygen XML for a single library.'''
  lib_path = os.path.join(_TRUNK, 'libraries', lib_dir)
  _LOG.info('Generating XML for library {0}'.format(lib_name))
  _LOG.info('{0} found at {1}'.format(lib_name, lib_path))
  xml_path = os.path.join(_XML, 'libraries', lib_dir)
  if not os.path.exists(xml_path):
    os.makedirs(xml_path)
  doxygen(
    INPUT='{0}/include {0}/source {0}/doc/Mainpage'.format(lib_path),
    PROJECT_NAME=lib_name,
    PROJECT_NUMBER='unknown',
    STRIP_FROM_PATH=lib_path,
    STRIP_FROM_INC_PATH='{0} {1}'.format(
      os.path.join(lib_path, 'include'),
      os.path.join(lib_path, 'source')
    ),
    XML_OUTPUT=xml_path
  )

def generate_xml():
  '''Generate doxygen XML for all libraries.'''
  for dirname, name in _LIBRARIES.iteritems():
    generate_library_xml(dirname, name)

def generate_library_pdf(lib_dir):
  '''Search for LaTeX files in lib_dir/latex, compile them and generate 
  an index file called articles.rst.'''
  texdir = os.path.join(_RST, 'libraries', lib_dir, 'latex')
  if os.path.exists(texdir):
    olddir = os.getcwd()
    os.chdir(texdir)
    _LOG.info('Compiling LaTeX documents for {0}'.format(lib_dir))
    titles = []
    try:
      for f in os.listdir('.'):
        if f.endswith('.tex'):
          fn = os.path.splitext(f)[0]
          titles.append(':download:`{0} <libraries/{2}/latex/{1}.pdf>`'.format(makepdf(fn), fn, lib_dir))
      open(os.path.join('..', 'articles.rst'), 'w+').write(
        _ARTICLEINDEXTEMPLATE.substitute(ARTICLES='* ' + '\n* '.join(titles))
      )
    finally:
      os.chdir(olddir)

def generate_library_rst(lib_dir):
  '''Generate reStructuredText documentation for a single library.'''
  xml_path = os.path.join(_XML, 'libraries', lib_dir)
  rst_path = os.path.join(_RST, 'libraries', lib_dir)
  refindex = os.path.join(rst_path, 'reference.rst')
  transform = os.path.join(_PWD, 'compound.xsl')
  if not os.path.exists(rst_path):
    os.makedirs(rst_path)
  classrst = []
  headerrst = []
  for f in os.listdir(xml_path):
    base, ext = os.path.splitext(f)
    if ext == '.xml' and base != "index":
      src = os.path.join(xml_path, f)
      dst = os.path.join(rst_path, base + '.rst')
      if base.startswith('class') and not base.startswith('classstd'):
        classrst.append(base)
        xsltproc(src, transform, dst, "'{0}'".format(xml_path))
        _LOG.info('Generated {0}'.format(dst))
      elif base.endswith('_8h'):
        headerrst.append(base)
        xsltproc(src, transform, dst, "'{0}'".format(xml_path))
        _LOG.info('Generated {0}'.format(dst))
  open(refindex, 'w+').write(_REFINDEXTEMPLATE.substitute(
    CLASSES='\n   '.join(classrst),
    FILES='\n   '.join(headerrst)
  ))
  _LOG.info('Generated {0}'.format(refindex))

def generate_rst():
  '''Generate reStructuredText documentation for all libraries.'''
  if os.path.exists(_XML):
    _LOG.info('Assuming Doxygen XML is up-to-date. Remove directory "{0}" to '
      're-generate.'.format(_XML))
  else:
    generate_xml()
  if os.path.exists(os.path.join(_RST, 'libraries', 'atermpp', 'reference.rst')):
      _LOG.info('Assuming reStructuredText is up-to-date. Remove directory '
        '"{0}" to re-generate.'.format(_RST))
  else:
    for dirname in _LIBRARIES:
      generate_library_rst(dirname)
  if os.path.exists(os.path.join(_RST, 'libraries', 'pbes', 'articles.rst')):
      _LOG.info('Assuming generated PDF is up-to-date. Remove directory '
        '"{0}" to re-generate.'.format(_RST))
  else:
    for dirname in _LIBRARIES:
      generate_library_pdf(dirname)
