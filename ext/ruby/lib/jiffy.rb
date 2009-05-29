#
# Jiffy - Fast, lighweight, and reentrant JSON stream parser.
#  
# Copyright (C) 2009 Paul Duncan <pabs@pablotron.org>
#  
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#   
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the of the
# Software.
#    
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  
#  

# load native library
require 'jiffy_parser'
require 'jiffy/document_parser'

module Jiffy
  class Parser
    def done
      parse(nil)
    end
  end

  def self.parse(str, &block)
    Parser.new do |p|
      p.parse(str, &block)
    end
  end

  def self.encode(val)
    DocumentParser.encode(val)
  end

  def self.decode(str)
    DocumentParser.decode(str)
  end
end
