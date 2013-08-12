module Rugged
  class Tag < Reference
    def name
      self.canonical_name.sub("refs/tags/", "")
    end

    def annotated?
      @owner.lookup(self.target).is_a?(TagAnnotation)
    end

    def annotation
      target = @owner.lookup(self.target)
      target.is_a?(TagAnnotation) ? target : nil
    end

    def target_object
      target = @owner.lookup(self.target)
      target.is_a?(TagAnnotation) ? target.target : target
    end

    def inspect
      "#<Rugged::Tag:#{object_id} {name: #{name.inspect}, target: #{target_object.inspect}, annotation:#{annotation.inspect} }>"
    end
  end
end
