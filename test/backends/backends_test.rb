require 'test_helper'

BACKENDS = [
  {type: :redis, host: 'localhost', port: 6379 }
]

class BackendTest < MiniTest::Unit::TestCase
  def setup
    @repos = BACKENDS.map do |backend_conf|
      Rugged::Repository.bare("test", backend: backend_conf)
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
      oid = repo.write("Test content", :blob)
      assert_match(/^[a-f0-9]+$/, oid)
    end
  end

  def test_can_test_existence
    skip("pending writing")
  end

  def test_can_read_back
    skip("pending writing")
  end

  def test_can_read_header
    skip("pending writing")
  end
end
