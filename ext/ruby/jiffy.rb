require 'jiffy.so'

module Jiffy
  def self.parse(str, &block)
    p = Parser.new
    p.parse(str, &block)
    p.parse(nil, &block)
  end
end
