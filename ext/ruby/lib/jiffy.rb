require 'jiffy.so'

module Jiffy
  module DocumentParser
    ESCAPE_RE = /([\b\f\t\n\r\v\\\/])/

    def self.encode(val, wrap = true)
      r = case val
      when String
        '"' + val.gsub(ESCAPE_RE) { |c|
          case c
          when "\b"
            '\\b'
          when "\f"
            '\\f'
          when "\n"
            '\\n'
          when "\r"
            '\\r'
          when "\t"
            '\\t'
          when "\v"
            '\\v'
          when "\\"
            '\\\\'
          when "/"
            '\\/'
          else
            # never reached
            raise Error, "unsupported character: #{c} (bug?)"
          end
        } + '"'
      when Numeric
        val.to_s
      when true
        val.to_s
      when false
        val.to_s
      when nil
        'null'
      when Array
        '[' + val.map { |v| encode(v, false) }.join(',') + ']'
      when Hash
        '{' + val.map { |k, v| 
          unless k.is_a?(String)
            raise Error, "non-string key values are not allowed"
          end

          [encode(k, false), encode(v, false)].join(':') 
        }.join(',') + '}'
      else
        raise Error, "invalid object type: #{val.class.name}"
      end

      # wrap result in parens
      r = '(' + r + ')' if wrap

      # return result
      r
    end
        
    class Node
      attr_accessor :parent, :value

      def initialize(parent = nil, value = nil)
        @parent, @value = parent, value
      end
    end

    def self.decode(str)
      keys = []
      node = Node.new
      
      push_node = proc do
        if node.parent
          case node.parent.class
          when Array
            node.parent << node.value
            node = node.parent
          when Hash
            if keys.last
              node.parent[keys.last] = node.value
              keys.last = nil
            elsif node.value.class == String
              keys.last = node.value
            else
              # XXX: shouldn't ever happen
              raise Error, "trying to use non-string value as key (bug?)"
            end

            node = node.parent
          else
            # XXX: shouldn't ever happen
            raise Error, "invalid node in stack (not array or hash, bug?)"
          end
        end
      end

      Parser.new do |p|
        p.parse(str) do |type, val|
          case type
          when TYPE_BGN_STRING
            node = Node.new(node)
            node.value = ''
          when TYPE_STRING_FRAGMENT
            node.value << val
          when TYPE_END_STRING
            push_node.call
          when TYPE_INTEGER
            node = Node.new(node, Integer(val))
            push_node.call
          when TYPE_INTEGER
            node = Node.new(node, Float(val))
            push_node.call
          when TYPE_TRUE
            node = Node.new(node, true)
            push_node.call
          when TYPE_FALSE
            node = Node.new(node, false)
            push_node.call
          when TYPE_NULL
            node = Node.new(node, nil)
            push_node.call
          when TYPE_BGN_OBJECT
            keys << nil
            node = Node.new(node, {})
          when TYPE_END_OBJECT
            keys.pop
            push_node.call
          when TYPE_BGN_ARRAY
            node = Node.new(node, [])
          when TYPE_END_ARRAY
            push_node.call
          else
            # XXX: shouldn't ever happen
            raise Error, "unknown node type: #{type} (bug?)"
          end
        end
      end

      # return result
      node.value
    end
  end

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
