module Rugged
  class ReferenceCollection
    def head
      self["HEAD"]
    end
  end
end
