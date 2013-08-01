module Rugged
  class ReferenceCollection
    def head
      self["HEAD"]
    end

    def each(&block)
      Reference.each(@owner, &block)
    end

    def glob(pattern, &block)
      Reference.each(@owner, pattern, &block)
    end

    def exists?(name)
      !!self[name]
    end
    alias :exist? :exists?

    def valid_name?(name)
      Reference.valid_name?(name)
    end

    def rename(ref_or_name, new_name, force = false)
      ref = ref_or_name if ref_or_name.kind_of?(Reference)
      ref ||= self[ref_or_name]

      unless ref
        raise InvalidError.new("Reference '#{ref_or_name}' doesn't exist. One cannot move a non existing reference.")
      end

      ref.rename(new_name, force)
    end

    def update(ref_or_name, target)
      ref = ref_or_name if ref_or_name.kind_of?(Reference)
      ref ||= self[ref_or_name]

      if ref.name == "HEAD"
        self.add("HEAD", target, true)
      else
        ref.set_target(target)
      end
    end

    def delete(ref_or_name)
      ref = ref_or_name if ref_or_name.kind_of?(Reference)
      ref ||= self[ref_or_name]
      ref.delete!
    end
  end
end
