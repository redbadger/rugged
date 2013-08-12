module Rugged
  class TagAnnotation
    def self.prettify_message(msg, strip_comments = true)
      Rugged::prettify_message(msg, strip_comments)
    end

    def inspect
      "#<Rugged::TagAnnotation:#{object_id} {name: #{name.inspect}, message: #{message.inspect}, target: #{target.inspect}>"
    end

    def to_hash
      {
        :message => message,
        :name => name,
        :target => target,
        :tagger => tagger,
      }
    end

    def modify(new_args, force = true)
      args = self.to_hash.merge(new_args)
      self.class.create(args, force)
    end
  end
end