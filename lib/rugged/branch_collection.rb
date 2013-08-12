module Rugged
  class BranchCollection
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
