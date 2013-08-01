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

    def rename(ref, new_name, force = false)
      ref = self[ref] if ref.kind_of?(String)
      ref.rename(new_name, force)
    end

    def update(ref, target)
      ref = self[ref] if ref.kind_of?(String)

      if ref.name == "HEAD"
        self.add("HEAD", target, true)
      else
        ref.set_target(target)
      end
    end

    def delete(ref)
      ref = self[ref] if ref.kind_of?(String)

      ref.delete!
    end
  end
end
