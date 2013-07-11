module Rugged
  class ReferenceWrapper
    def initialize(repo, ref)
      @repo = repo
      @ref = ref
    end

    def canonical_name
      @ref.name
    end

    alias :name :canonical_name

    def target

    end
  end

  class Branch < ReferenceWrapper
    def [](path)
      tip && tip[path]
    end

    def checkout(options = {})
      @repo.checkout(self.name, options)
    end

    def name
      @ref.name.sub(%r{^/refs/(heads|remotes)/}, "")
    end

    def remote?
      @ref.remote?
    end

    def head?
      @ref.head?
    end

    def target
      Rugged::Commit.lookup(@repo, @ref.resolve.target)
    end
    alias :target :tip
  end

  class Tag < ReferenceWrapper
    def annotated?

    end

    def name

    end

    def message

    end

    def tagger

    end

    def target
      # git_tag_peel
    end
    alias :target :annotation

    def name
      @ref.name.sub(%r{^/refs/tags/})
    end
  end
end
