/*
 * The MIT License
 *
 * Copyright (c) 2013 GitHub, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "rugged.h"

extern VALUE rb_mRugged;
extern VALUE rb_mRuggedRepo;
extern VALUE rb_cRuggedBranch;
VALUE rb_cRuggedBranchCollection;

static VALUE rb_git_branch_collection_initialize(VALUE self, VALUE repo) {
	rugged_set_owner(self, repo);
	return self;
}

static VALUE rb_git_branch_collection_aref(VALUE self, VALUE name) {
	VALUE rb_repo = rugged_owner(self);
	git_repository *repo;
	git_reference *branch;
	int error;

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_branch_lookup(&branch, repo, StringValueCStr(name), GIT_BRANCH_LOCAL);
	if (error == GIT_ENOTFOUND)
		error = git_branch_lookup(&branch, repo, StringValueCStr(name), GIT_BRANCH_REMOTE);

	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedBranch, rb_repo, branch);
}


static int cb_branch__each_name(const char *branch_name, git_branch_t branch_type, void *data)
{
	struct rugged_cb_payload *payload = data;

	rb_protect(rb_yield, rb_str_new_utf8(branch_name), &payload->exception);

	return payload->exception ? GIT_ERROR : GIT_OK;
}

static int cb_branch__each_obj(const char *branch_name, git_branch_t branch_type, void *data)
{
	git_reference *branch;
	git_repository *repo;
	struct rugged_cb_payload *payload = data;

	Data_Get_Struct(payload->rb_data, git_repository, repo);

	rugged_exception_check(
		git_branch_lookup(&branch, repo, branch_name, branch_type)
	);

	rb_protect(rb_yield, rugged_branch_new(payload->rb_data, branch), &payload->exception);
	return payload->exception ? GIT_ERROR : GIT_OK;
}

static VALUE each_branch(int argc, VALUE *argv, VALUE self, int branch_names_only)
{
	VALUE rb_repo = rugged_owner(self), rb_filter;
	git_repository *repo;
	int error;
	struct rugged_cb_payload payload;
	int filter = (GIT_BRANCH_LOCAL | GIT_BRANCH_REMOTE);

	rb_scan_args(argc, argv, "01", &rb_filter);

	payload.exception = 0;
	payload.rb_data = rb_repo;

	if (!rb_block_given_p()) {
		VALUE symbol = branch_names_only ? CSTR2SYM("each_name") : CSTR2SYM("each");
		return rb_funcall(self, rb_intern("to_enum"), 2, symbol, rb_filter);
	}

	rugged_check_repo(rb_repo);

	if (!NIL_P(rb_filter)) {
		ID id_filter;

		Check_Type(rb_filter, T_SYMBOL);
		id_filter = SYM2ID(rb_filter);

		if (id_filter == rb_intern("local")) {
			filter = GIT_BRANCH_LOCAL;
		} else if (id_filter == rb_intern("remote")) {
			filter = GIT_BRANCH_REMOTE;
		} else {
			rb_raise(rb_eTypeError,
				"Invalid branch filter. Expected `:remote`, `:local` or `nil`");
		}
	}

	Data_Get_Struct(rb_repo, git_repository, repo);

	if (branch_names_only) {
		error = git_branch_foreach(repo, filter, &cb_branch__each_name, &payload);
	} else {
		error = git_branch_foreach(repo, filter, &cb_branch__each_obj, &payload);
	}

	if (payload.exception)
		rb_jump_tag(payload.exception);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    branches.each(repository, filter = nil) { |branch| block }
 *    branches.each(repository, filter = nil) -> enumerator
 *
 *  Iterate through the branches in +repository+. Iteration can be
 *  optionally filtered to yield only +:local+ or +:remote+ branches.
 *
 *  The given block will be called once with a +Rugged::Branch+ object
 *  for each branch in the repository. If no block is given, an enumerator
 *  will be returned.
 */
static VALUE rb_git_branch_collection_each(int argc, VALUE *argv, VALUE self)
{
	return each_branch(argc, argv, self, 0);
}

/*
 *  call-seq:
 *    branches.each_name(repository, filter = nil) { |branch_name| block }
 *    branches.each_name(repository, filter = nil) -> enumerator
 *
 *  Iterate through the names of the branches in +repository+. Iteration can be
 *  optionally filtered to yield only +:local+ or +:remote+ branches.
 *
 *  The given block will be called once with the name of each branch as a +String+.
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_branch_collection_each_name(int argc, VALUE *argv, VALUE self)
{
	return each_branch(argc, argv, self, 1);
}

void Init_rugged_branch_collection(void)
{
	rb_cRuggedBranchCollection = rb_define_class_under(rb_mRugged, "BranchCollection", rb_cObject);
	rb_include_module(rb_cRuggedBranchCollection, rb_mEnumerable);

	rb_define_method(rb_cRuggedBranchCollection, "initialize", rb_git_branch_collection_initialize, 1);
	rb_define_method(rb_cRuggedBranchCollection, "[]", rb_git_branch_collection_aref, 1);
	rb_define_method(rb_cRuggedBranchCollection, "each", rb_git_branch_collection_each, -1);
	rb_define_method(rb_cRuggedBranchCollection, "each_name", rb_git_branch_collection_each_name, -1);
	//rb_define_method(rb_cRuggedBranchCollection, "add", rb_git_branch_collection_add, -1);
	// rb_define_method(rb_cRuggedBranchCollection, "rename", rb_git_branch_collection_rename, -1);
	// rb_define_method(rb_cRuggedBranchCollection, "update", rb_git_branch_collection_update, 2);
	// rb_define_method(rb_cRuggedBranchCollection, "delete", rb_git_branch_collection_delete, 1);
}
