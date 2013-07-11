require 'test_helper'

class ReferenceCollectionTest < Rugged::SandboxedTestCase
  def setup
    super
    sandbox_init("testrepo.git")
    @repo = sandbox_clone("testrepo.git", "cloned_repo")
  end

  def test_lookup
    ref = @repo.references["refs/heads/master"]
    assert_instance_of Rugged::Reference, ref
    assert_equal "refs/heads/master", ref.name

    ref = @repo.references["refs/remotes/origin/master"]
    assert_instance_of Rugged::Reference, ref
    assert_equal "refs/remotes/origin/master", ref.name

    assert_nil @repo.references["refs/does/not/exist"]
  end

  def test_exists?
    assert @repo.references.exists?("refs/heads/master")
    refute @repo.references.exists?("refs/does/not/exist")
  end

  def test_each
    names = []

    @repo.references.each do |ref|
      names << ref.name
    end

    assert_equal [
      "refs/heads/master",
      "refs/remotes/origin/HEAD",
      "refs/remotes/origin/master",
      "refs/remotes/origin/packed",
      "refs/tags/v0.9",
      "refs/tags/v1.0",
    ], names.sort
  end

  def test_glob
    names = []

    @repo.references.glob("refs/remotes/*") do |ref|
      names << ref.name
    end

    assert_equal [
      "refs/remotes/origin/HEAD",
      "refs/remotes/origin/master",
      "refs/remotes/origin/packed",
    ], names.sort
  end

  def test_head
    head = @repo.references.head
    assert_equal "HEAD", head.name
    assert_equal "refs/heads/master", head.target
  end

  def test_add_symbolic_ref

  end

  def test_add_direct_ref

  end

  def test_add_overwrite_existing_ref

  end

  def test_update_with_name

  end

  def test_update_with_ref

  end

  def test_update_can_detach_HEAD
  end

  def test_rename_with_name
    @repo.references.rename("refs/remotes/origin/packed", "refs/heads/packed")

    refute @repo.references.exists?("refs/remotes/origin/packed")
    assert @repo.references.exists?("refs/heads/packed")
  end

  def test_rename_with_ref
    @repo.references.rename(@repo.references["refs/remotes/origin/packed"], "refs/heads/packed")

    refute @repo.references.exists?("refs/remotes/origin/packed")
    assert @repo.references.exists?("refs/heads/packed")
  end

  def test_delete_with_name
    @repo.references.delete("refs/remotes/origin/packed")
    refute @repo.references.exists?("refs/remotes/origin/packed")
  end

  def test_delete_with_ref
    @repo.references.delete(@repo.references["refs/remotes/origin/packed"])
    refute @repo.references.exists?("refs/remotes/origin/packed")
  end
end
