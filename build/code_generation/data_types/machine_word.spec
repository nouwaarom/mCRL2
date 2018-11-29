% Author(s): Jan Friso Groote
% Copyright: see the accompanying file COPYING or copy at
% https://github.com/mCRL2org/mCRL2/blob/master/COPYING
%
% Distributed under the Boost Software License, Version 1.0.
% (See accompanying file LICENSE_1_0.txt or copy at
% http://www.boost.org/LICENSE_1_0.txt)
%
% Declaration of the sort @Word, that contains machine words. The operations
% on machine words are defined using explicit C++ code. 

#include bool.spec

sort @word <"machine_word">;
cons @zero_word <"zero64">: @word                                                                                                                   internal defined_by_code;
     @succ_word <"succ64">: @word <"arg">-> @word                                                                                                   internal defined_by_code;

%% Core functions that are used by other datatypes.
map  @one_word <"one_word">: @word                                                                                                                  internal defined_by_code;
     @max_word <"max_word">: @word                                                                                                                  internal defined_by_code;

     @add_word <"add_word">: @word <"left"> # @word <"right"> -> @word                                                                              internal defined_by_code; 
     @add_overflow_word <"add_overflow">: @word <"left"> # @word <"right"> -> @word                                                                 internal defined_by_code; 
     @times_word <"times_word">: @word <"left"> # @word <"right"> -> @word                                                                          internal defined_by_code; 
     @times_overflow_word <"timew_overflow_word">: @word <"left"> # @word <"right"> -> @word                                                        internal defined_by_code; 
     @minus_word <"minus_word">: @word <"left"> # @word <"right"> -> @word                                                                          internal defined_by_code; 
     @div_word <"div_word">: @word <"left"> # @word <"right"> -> @word                                                                              internal defined_by_code; 
     @mod_word <"mod_word">: @word <"left"> # @word <"right"> -> @word                                                                              internal defined_by_code;
     @sqrt_word <"sqrt_word">: @word <"arg"> -> @word                                                                                               internal defined_by_code;                            
     @div_doubleword <"div_doubleword">: @word <"arg1"> # @word <"arg2"> # @word <"arg3"> -> @word                                                  internal defined_by_code;       
     @div_double_doubleword <"div_double_doubleword">: @word <"arg1"> # @word <"arg2"> # @word <"arg3"> # @word <"arg4"> -> @word                   internal defined_by_code; 
     @div_triple_doubleword <"div_triple_doubleword">: @word <"arg1"> # @word <"arg2"> # @word <"arg3"> # @word <"arg4"> # @word <"arg5"> -> @word  internal defined_by_code; 
     @mod_doubleword <"mod_doubleword">: @word <"arg1"> # @word <"arg2"> # @word <"arg3"> -> @word                                                  internal defined_by_code;       
     @sqrt_doubleword <"sqrt_doubleword">: @word <"arg1"> # @word <"arg2"> -> @word                                                                 internal defined_by_code; 
     @sqrt_tripleword <"sqrt_tripleword">: @word <"arg1"> # @word <"arg2"> # @word <"arg3"> -> @word                                                internal defined_by_code;      
     @sqrt_tripleword_overflow <"sqrt_tripleword_overflow">: @word <"arg1"> # @word <"arg2"> # @word <"arg3"> -> @word                              internal defined_by_code;   
     @sqrt_quadrupleword <"sqrt_quadrupleword">: @word <"arg1"> # @word <"arg2"> # @word <"arg3"> # @word <"arg4"> -> @word                         internal defined_by_code;  
     @sqrt_quadrupleword_overflow <"sqrt_quadrupleword_overflow">: @word <"arg1"> # @word <"arg2"> # @word <"arg3"> # @word <"arg4"> -> @word       internal defined_by_code;  
     @pred_word <"pred_word">: @word <"arg"> ->@word                                                                                                internal defined_by_code;   

var d:@word;
eqn ==(d, d) = true;