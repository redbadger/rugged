module Rugged
  class BranchCollection
    def add(name, target, force = false)
      Branch.create(@owner, name, target, force)
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
