require "test_helper"

class TagTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def test_reading_a_tag
    oid = "0c37a5391bbff43c37f0d0371823a5509eed5b1d"
    obj = @repo.lookup(oid)

    assert_equal oid, obj.oid
    assert_equal :tag, obj.type
    assert_equal "test tag message\n", obj.message
    assert_equal "v1.0", obj.name
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", obj.target.oid
    assert_equal :commit, obj.target_type
    c = obj.tagger
    assert_equal "Scott Chacon", c[:name]
    assert_equal 1288114383, c[:time].to_i
    assert_equal "schacon@gmail.com", c[:email]
  end

  def test_reading_the_oid_of_a_tag
    oid = "0c37a5391bbff43c37f0d0371823a5509eed5b1d"
    obj = @repo.lookup(oid)

    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", obj.target_oid
  end

  def test_create_heavy_tag
    @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644", {
      :message => "test tag message\n",
      :tagger  => {
        :name  => 'Scott',
        :email => 'schacon@gmail.com',
        :time  => Time.now
      }
    })

    tag = @repo.tags["tag"]

    assert tag.annotated?
    assert_equal "test tag message\n", tag.annotation.message
    assert_equal 'Scott', tag.annotation.tagger[:name]
    assert_equal 'schacon@gmail.com', tag.annotation.tagger[:email]
  end

  def test_create_lightweight_tag
    @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    tag = @repo.tags["tag"]

    refute tag.annotated?
    assert_nil tag.annotation
  end

  def test_deleting_tag_with_short_name
    @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    @repo.tags.delete("tag")
    refute @repo.tags["tag"]
  end

  def test_deleting_tag_with_canonical_name
    @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    @repo.tags.delete("refs/tags/tag")
    refute @repo.tags["tag"]
  end

  def test_deleting_tag_with_tag_object
    @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    @repo.tags.delete(@repo.tags["tag"])
    refute @repo.tags["tag"]
  end

  def test_lookup_tag_by_canonical_name
    @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    assert_equal "tag", @repo.tags["refs/tags/tag"].name
  end
end

class TagWriteTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def test_creating_tag_annotation
    tag_oid = Rugged::TagAnnotation.create(@repo, {
      :name    => "new_tag",
      :target  => "5b5b025afb0b4c913b4c338a42934a3863bf3644",
      :message => "test tag message\n",
      :tagger  => {
        :name  => 'Scott',
        :email => 'schacon@gmail.com',
        :time  => Time.now
      }
    })

    tag = @repo.lookup(tag_oid)
    assert_equal :tag, tag.type
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", tag.target.oid
    assert_equal "test tag message\n", tag.message
    assert_equal "Scott", tag.tagger[:name]
    assert_equal "schacon@gmail.com", tag.tagger[:email]
  end

  def test_creating_tag_annotation_does_not_create_ref
    Rugged::TagAnnotation.create(@repo, {
      :name    => "new_tag",
      :target  => "5b5b025afb0b4c913b4c338a42934a3863bf3644",
      :message => "test tag message\n",
      :tagger  => {
        :name  => 'Scott',
        :email => 'schacon@gmail.com',
        :time  => Time.now
      }
    })

    refute @repo.references["refs/tags/new_tag"]
  end

  def test_writing_a_tag
    tag_oid = @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644", {
      :message => "test tag message\n",
      :tagger  => {
        :name  => 'Scott',
        :email => 'schacon@gmail.com',
        :time  => Time.now
      }
    })

    tag = @repo.lookup(tag_oid)
    assert_equal :tag, tag.type
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", tag.target.oid
    assert_equal "test tag message\n", tag.message
    assert_equal "Scott", tag.tagger[:name]
    assert_equal "schacon@gmail.com", tag.tagger[:email]
  end

  def test_tag_invalid_message_type
    assert_raises TypeError do
      @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644", {
        :message => :invalid_message,
        :tagger  => {
          :name => 'Scott',
          :email => 'schacon@gmail.com',
          :time => Time.now
        }
      })
    end
  end

  def test_writing_light_tags
    @repo.tags.create('tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    tag = @repo.references["refs/tags/tag"]
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", tag.target
  end
end
