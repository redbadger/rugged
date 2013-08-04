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

    def each(&block)
      Tag.each(@repo, &block)
    end
    
    def add(name, target, options = {})
      Reference.create(@repo, target: target, tagger: options[:tagger], message: options[:message], force: options[:force])
    end

    def delete(tag)
      tag = self[tag] if tag.kind_of?(String)
      tag.delete!
    end
  end
end
