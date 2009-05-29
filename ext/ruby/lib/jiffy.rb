require 'jiffy.so'

module Jiffy
  def self.parse(str, &block)
    Parser.new do |p|
      p.parse(str, &block)
    end
  end

  class Parser
    def done
      parse(nil)
    end
  end
end
