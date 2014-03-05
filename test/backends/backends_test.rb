require 'test_helper'

BACKENDS = [
  {type: :redis, host: 'localhost', port: 6379 }
]

class BackendTest < MiniTest::Unit::TestCase
  def setup
    @repos = BACKENDS.map do |backend_conf|
      Rugged::Repository.bare("test_repo", backend: backend_conf)
    end
  end

  def each_backend(&block)
    @repos.each do |repo|
      yield(repo)
    end
  end
end

# do not inherit from Rugged::TestCase, we're not touching the disk
class OdbTest < BackendTest
  def test_can_write
    each_backend do |repo|
      oid = repo.write("Test", :blob)
      assert_match(/^[a-f0-9]+$/, oid)
    end
  end

  def test_can_test_existence
    each_backend do |repo|
      oid1 = "1000000000000000000000000000000000000001"
      assert_equal(false, repo.exists?(oid1))

      oid2 = repo.write("Test content", :blob)
      assert_equal(true, repo.exists?(oid2))
    end
  end

  def test_can_read_back
    each_backend do |repo|
      oid = repo.write("Another test content", :blob)

      object = repo.read(oid)

      assert_equal(:blob, object.type)
      assert_equal("Another test content", object.data)
    end
  end

  def test_can_read_header
    each_backend do |repo|
      oid = repo.write("12345", :blob)

      header = repo.read_header(oid)

      assert_equal(:blob, header[:type])
      assert_equal(5, header[:len])
    end
  end
end

class RefdbTest < BackendTest
  def teardown
    each_backend do |repo|
      [
        "HEAD",
        "refs/heads/test",
        "refs/heads/master",
        "refs/heads/test1",
        "refs/heads/test2"
      ].each do |ref_name|
        ref = Rugged::Reference.lookup(repo, ref_name)
        ref.delete! if ref
      end
    end
  end

  def test_can_lookup
    each_backend do |repo|
      ref = Rugged::Reference.lookup(repo, "refs/heads/nope")

      assert_nil(ref)

      oid = repo.write("Test object for reference", :blob)
      Rugged::Reference.create(repo, "refs/heads/test", oid)
      ref = Rugged::Reference.lookup(repo, "refs/heads/test")

      refute_nil(ref)

      assert_equal(oid, ref.target)
      assert_equal(:direct, ref.type)
      assert_equal("refs/heads/test", ref.name)
    end
  end

  def test_can_lookup_symbolic
    each_backend do |repo|
      oid = repo.write("Test object for reference", :blob)
      Rugged::Reference.create(repo, "refs/heads/master", oid)
      ref = Rugged::Reference.lookup(repo, "HEAD")

      refute_nil(ref)

      assert_equal("refs/heads/master", ref.target)
      assert_equal(:symbolic, ref.type)
      assert_equal("HEAD", ref.name)
    end
  end

  def test_can_rename
    each_backend do |repo|
      oid = repo.write("Test object for reference", :blob)
      ref = Rugged::Reference.create(repo, "refs/heads/test1", oid)

      new_ref = ref.rename("refs/heads/test2")

      refute_nil(new_ref)
      assert_equal("refs/heads/test2", new_ref.name)
      assert_equal(:direct, new_ref.type)
      assert_equal(oid, new_ref.target)
    end
  end

  def test_can_delete
    each_backend do |repo|
      oid = repo.write("Test object for reference", :blob)
      ref = Rugged::Reference.create(repo, "refs/heads/test", oid)

      ref.delete!
      new_ref = Rugged::Reference.lookup(repo, "refs/heads/test")

      assert_nil(new_ref);
    end
  end

  def test_can_iterate_refs
    each_backend do |repo|
      assert_kind_of(Enumerator, repo.refs)

      n = 0
      Rugged::Reference.each(repo) do |ref|
        assert_kind_of(Rugged::Reference, ref)
        n += 1
      end

      assert_operator(1, :<=, n)
    end
  end

  def test_can_iterate_ref_names
    each_backend do |repo|
      assert_kind_of(Enumerator, repo.refs)

      n = 0
      Rugged::Reference.each_name(repo) do |ref|
        assert_kind_of(String, ref)
        assert_match(/^[a-zA-Z\/]+$/, ref)
        n += 1
      end

      assert_operator(1, :<=, n)
    end
  end

  def test_can_iterate_refs_with_glob
    refs = ["refs/heads/master", "refs/heads/test1", "refs/heads/test2"]
    exp = ["refs/heads/test1", "refs/heads/test2"]

    each_backend do |repo|
      oid = repo.write("Test object for reference", :blob)
      refs.each {|ref| Rugged::Reference.create(repo, ref, oid) }

      names = []
      Rugged::Reference.each_name(repo, "refs/heads/test*") { |name| names << name }

      assert_equal(exp, names.sort)
    end
  end

  def test_can_iterate_ref_names_with_glob
    refs = ["refs/heads/master", "refs/heads/test1", "refs/heads/test2"]
    exp = ["refs/heads/test1", "refs/heads/test2"]

    each_backend do |repo|
      oid = repo.write("Test object for reference", :blob)
      refs.each {|ref| Rugged::Reference.create(repo, ref, oid) }

      names = repo.refs("refs/heads/test*").map { |r| r.name }

      assert_equal(exp, names.sort)
    end
  end

end

