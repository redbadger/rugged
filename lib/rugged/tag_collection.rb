module Rugged
  class TagCollection
    include Enumerable

    def initialize(repo)
      @repo = repo
    end

    def [](name)
      name = "refs/tags/#{name}" unless name.start_with?("refs/tags/")
      Tag.new(@repo, @repo.references[name])
    end

    def head
      self["HEAD"]
    end

    def each(&block)
      Tag.each(@repo, &block)
    end
    
    def add(name, target, tagger = nil, message = nil, force = false)
      Reference.create(@repo, target: target, tagger: tagger, message: message, force: force)
    end

    def delete(tag)
      tag = self[tag] if tag.kind_of?(String)
      tag.delete!
    end
  end
end
