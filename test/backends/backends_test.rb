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

  def teardown

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
