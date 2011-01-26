# Author(s): Wieger Wesselink
# Copyright: see the accompanying file COPYING or copy at
# https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
# 
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
# 
#  \file wiki_tool_table.py
#  \brief Generate tool table in wikimedia format, see http://www.mcrl2.org/mcrl2/wiki/index.php/Private:Tool_Status_Overview

from path import *
import re
import string
import os

# == Release tools ==
# 
# {| class="wikitable" align="center"
# |-
# ! Tool !! Uses tool classes
# |-
# ! colspan="2" style="text-align: left" | Squadt
# |-
# | [[User manual/squadt|squadt]]                                     ||align="center"| not applicable
# |-
# | [[User manual/formulacheck|formulacheck]]                         ||align="center"| -
# |-                                                                  
# | [[User manual/lpsactionrename|lpsactionrename]]                   ||align="center"| +
# |-                                                                  
# | [[User manual/lpsbinary|lpsbinary]]                               ||align="center"| +
# |-                                                                  
# | [[User manual/lpsconfcheck|lpsconfcheck]]                         ||align="center"| -
# |-                                                                  
# | [[User manual/lpsconstelm|lpsconstelm]]                           ||align="center"| +
# |-                                                                  
# | [[User manual/lpsinfo|lpsinfo]]                                   ||align="center"| -
# |-                                                                  
# | [[User manual/lpsinvelm|lpsinvelm]]                               ||align="center"| -
# |-                                                                  
# | [[User manual/lpsparelm|lpsparelm]]                               ||align="center"| +
# |-                                                                  
# | [[User manual/lpspp|lpspp]]                                       ||align="center"| -
# |-                                                                  
# | [[User manual/lpsrewr|lpsrewr]]                                   ||align="center"| +
# |-                                                                  
# | [[User manual/lpssumelm|lpssumelm]]                               ||align="center"| +
# |-                                                                  
# | [[User manual/lpssuminst|lpssuminst]]                             ||align="center"| +
# |-                                                                  
# | [[User manual/lpsuntime|lpsuntime]]                               ||align="center"| +
# |-                                                                  
# | [[User manual/mcrl22lps|mcrl22lps]]                               ||align="center"| -
# |-                                                                  
# | [[User manual/sim|sim]]                                           ||align="center"| -
# |-                                                                  
# | [[User manual/xsim|xsim]]                                         ||align="center"| -
# |-
# | [[User manual/lps2pbes|lps2pbes]]                                 ||align="center"| +
# |-                                                                  
# | [[User manual/pbes2bool|pbes2bool]]                               ||align="center"| +
# |-                                                                  
# | [[User manual/pbesinfo|pbesinfo]]                                 ||align="center"| -
# |-                                                                  
# | [[User manual/pbespp|pbespp]]                                     ||align="center"| -
# |-                                                                  
# | [[User manual/pbesrewr|pbesrewr]]                                 ||align="center"| +
# |-                                                                  
# | [[User manual/txt2pbes|txt2pbes]]                                 ||align="center"| -
# |-
# | [[User manual/diagraphica|diagraphica]]                           ||align="center"| not applicable
# |-                                                                  
# | [[User manual/lps2lts|lps2lts]]                                   ||align="center"| -
# |-                                                                  
# | [[User manual/ltscompare|ltscompare]]                             ||align="center"| -
# |-                                                                  
# | [[User manual/ltsconvert|ltsconvert]]                             ||align="center"| -
# |-                                                                  
# | [[User manual/ltsgraph|ltsgraph]]                                 ||align="center"| not applicable
# |-                                                                  
# | [[User manual/ltsinfo|ltsinfo]]                                   ||align="center"| -
# |-                                                                  
# | [[User manual/ltsview|ltsview]]                                   ||align="center"| not applicable
# |-                                                                  
# | [[User manual/tracepp|tracepp]]                                   ||align="center"| -
# |-
# | [[User manual/chi2mcrl2|chi2mcrl2]]                               ||align="center"| -
# |-                                                                  
# | [[User manual/lps2torx|lps2torx]]                                 ||align="center"| -
# |-                                                                  
# | [[User manual/tbf2lps|tbf2lps]]                                   ||align="center"| -
# |}

TEXT = '''== Tools ==

{| class="wikitable" align="center"
|-
! Tool !! Uses tool classes || Has regression tests || Not using libraries || Tool status || Has squadt
TOOLTEXT|}
'''

TOOLITEM = '''|-
| [[User manual/TOOL|TOOL]] ||align="center" COLOR1| VALUE1 ||align="center"| VALUE2 ||align="center" COLOR3| VALUE3 || STATUS ||align="center"| HAS_SQUADT
'''

TOOLS = '''
chi2mcrl2         | +               | ? | ? (Process Library)
diagraphica       | +               | ? | not applicable
formulacheck      | +               | ? | -
grape             | +               | ? | -
lps2lts           | +               | ? | -
lps2pbes          | +               | ? | +
lps2torx          | +               | ? | -
lpsactionrename   | +               | ? | +
lpsbinary         | +               | ? | +
lpsbisim2pbes     | +               | ? | +
lpsconfcheck      | +               | ? | +
lpsconstelm       | +               | ? | +
lpsinfo           | +               | ? | +
lpsinvelm         | +               | ? | +
lpsparelm         | +               | ? | +
lpsparunfold      | +               | ? | +
lpspp             | +               | ? | -
lpsrealelm        | +               | ? | +
lpsrewr           | +               | ? | +
lpssumelm         | +               | ? | +
lpssuminst        | +               | ? | +
lpsuntime         | +               | ? | +
ltscompare        | +               | ? | ?
ltsconvert        | +               | ? | ?
ltsgraph          | +               | ? | not applicable
ltsinfo           | +               | ? | ?
ltsmin            | +               | ? | ?
ltsview           | +               | ? | ?
lysa2mcrl2        | +               | ? | +
mcrl22lps         | +               | ? | +
mcrl2i            | +               | ? | +
pbes2bes          | +               | ? | +
pbes2bool         | +               | ? | +
pbesconstelm      | +               | ? | +
pbesinfo          | +               | ? | +
pbesparelm        | +               | ? | +
pbespareqelm      | +               | ? | +
pbespgsolve       | +               | ? | +
pbespp            | +               | ? | -
pbesrewr          | +               | ? | +                                                              
sim               | +               | ? | -                                                                      
squadt            | +               | ? | not applicable
tbf2lps           | +               | ? | -                                                               
tracepp           | +               | ? | ?
txt2lps           | +               | ? | +
txt2pbes          | +               | ? | -
xsim              | +               | ? | -                                                                      
'''                                                                                            

text = ''
tools = re.split('\n', TOOLS)
for line in tools:
    line = string.strip(line)
    if len(line) == 0:
        continue
    words = map(string.strip, line.split('|'))

    tool = words[0].strip()
    if tool == 'sim':
      tooldir = 'xsim'
    else:
      tooldir = tool

    jamfile = path('../../tools').joinpath(tooldir).joinpath('Jamfile.v2').normcase().text()
    jamfile = re.search(r'(tool\s+%s[^;]*;)' % tool, jamfile).group(1)
    m = re.search(r'<status>(\w*)', jamfile)
    if m == None:
      status = 'release'
    else:
      status = m.group(1)
    m = re.search('<squadt-connectivity>implemented', jamfile)
    if m == None:
      has_squadt = 'no'
    else:
      has_squadt = 'yes'

    item = TOOLITEM
    if words[1] == '-':
        color1 = 'bgcolor="red"'
    elif words[1] == '+':
        color1 = 'bgcolor="lightgreen"'
    else:
        color1 = ''
    if words[1] in '+-':
        words[1] = ''
    if words[3] == '-':
        color3 = 'bgcolor="red"'
    elif words[3] == '+':
        color3 = 'bgcolor="lightgreen"'
    else:
        color3 = ''
    if words[3] in '+-':
        words[3] = ''
    item = re.sub('TOOL'  , words[0], item)
    item = re.sub('COLOR1', color1, item)
    item = re.sub('COLOR3', color3, item)
    item = re.sub('VALUE1', words[1], item)
    item = re.sub('VALUE2', words[2], item)
    item = re.sub('VALUE3', words[3], item)
    item = re.sub('STATUS', status, item)
    item = re.sub('HAS_SQUADT', has_squadt, item)
    text = text + item
text = re.sub('TOOLTEXT', text, TEXT)

# create subdirectory output if it doesn't exist
if not os.path.exists('output'):
    os.makedirs('output')
path('output/Tool_Status_Overview').write_text(text)
