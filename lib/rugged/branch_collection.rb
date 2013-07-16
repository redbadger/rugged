module Rugged
  class BranchCollection
    def initialize(repo)
      @repo = repo
    end

    def [](name)
      if name == "HEAD" || name.start_with?("refs/heads/") || name.start_with?("refs/remotes/")

      end

      # "refs/heads/#{name}"
      # "refs/remotes/#{name}"
      # "refs/#{name}"

      # name = "refs/tags/#{name}" unless name.start_with?("refs/tags/")
      # Tag.new(@repo, @repo.references[name])
    end

    def add(name, target, force = false)
      Branch.create(@repo, name, target, force)
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

    def delete(branch)
      branch = self[branch] if branch.kind_of?(String)

      branch.delete!
    end
  end
end
