module Rugged
  class Reference
    alias_method :canonical_name, :name

    def inspect
      "#<Rugged::Reference:#{object_id} {name: #{name.inspect}, target: #{target.inspect}}>"
    end

    def tag?
      name.start_with?("refs/tags/")
    end

    def note?
      name.start_with?("refs/notes/")
    end

    def local?
      name.start_with?("refs/heads/")
    end
  end
end
