module Rugged
  class ReferenceCollection
    include Enumerable

    def initialize(repo)
      @repo = repo
    end

    def [](name)
      Reference.lookup(@repo, name) rescue nil
    end

    def head
      self["HEAD"]
    end

    def each(&block)
      Reference.each(@repo, &block)
    end

    def glob(pattern, &block)
      Reference.each(@repo, pattern, &block)
    end

    def exists?(name)
      Reference.exists?(@repo, name)
    end

    def valid_name?(name)
      Reference.valid_name?(name)
    end

    alias :exist? :exists?

    def add(name, target, force = false)
      Reference.create(@repo, name, target, force)
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
