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
extern VALUE rb_cRuggedRepo;
extern VALUE rb_cRuggedObject;
extern VALUE rb_cRuggedReference;
VALUE rb_cRuggedBranch;

inline VALUE rugged_branch_new(VALUE owner, git_reference *ref)
{
	return rugged_ref_new(rb_cRuggedBranch, owner, ref);
}

static int parse_branch_type(VALUE rb_filter)
{
	ID id_filter;

	Check_Type(rb_filter, T_SYMBOL);
	id_filter = SYM2ID(rb_filter);

	if (id_filter == rb_intern("local")) {
		return GIT_BRANCH_LOCAL;
	} else if (id_filter == rb_intern("remote")) {
		return GIT_BRANCH_REMOTE;
	} else {
		rb_raise(rb_eTypeError,
			"Invalid branch filter. Expected `:remote`, `:local` or `nil`");
	}
}

/*
 *  call-seq:
 *    Branch.lookup(repository, name, branch_type = :local) -> branch
 *
 *  Lookup a branch in +repository+, with the given +name+.
 *
 *  +name+ needs to be a branch name, not an absolute reference path
 *  (e.g. +development+ instead of +/refs/heads/development+).
 *
 *  +branch_type+ specifies whether the looked up branch is a local branch
 *  or a remote one. It defaults to looking up local branches.
 *
 *  Returns the looked up branch, or +nil+ if the branch doesn't exist.
 */
static VALUE rb_git_branch_lookup(int argc, VALUE *argv, VALUE self)
{
	git_reference *branch;
	git_repository *repo;

	VALUE rb_repo, rb_name, rb_type;
	int error;
	int branch_type = GIT_BRANCH_LOCAL;

	rb_scan_args(argc, argv, "21", &rb_repo, &rb_name, &rb_type);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	if (!NIL_P(rb_type))
		branch_type = parse_branch_type(rb_type);

	error = git_branch_lookup(&branch, repo, StringValueCStr(rb_name), branch_type);
	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);
	return rugged_branch_new(rb_repo, branch);
}

/*
 *  call-seq:
 *    branch.head? -> true or false
 *
 *  Returns +true+ if the branch is pointed at by +HEAD+, +false+ otherwise.
 */
static VALUE rb_git_branch_head_p(VALUE self)
{
	git_reference *branch;
	Data_Get_Struct(self, git_reference, branch);
	return git_branch_is_head(branch) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *    branch.name -> string
 *
 *  Returns the name of +branch+.
 *
 *  See Rugged::Branch#canonical_name if you need the fully qualified
 *  name of the underlying reference.
 */
static VALUE rb_git_branch_name(VALUE self)
{
	git_reference *branch;
	const char *branch_name;
	Data_Get_Struct(self, git_reference, branch);

	git_branch_name(&branch_name, branch);
	return rb_str_new_utf8(branch_name);
}

static VALUE rb_git_branch__remote_name(VALUE rb_repo, const char *canonical_name)
{
	git_repository *repo;

	char *remote_name;
	size_t remote_name_size;
	int error;
	VALUE result = Qnil;

	Data_Get_Struct(rb_repo, git_repository, repo);

	remote_name_size = git_branch_remote_name(NULL, 0, repo, canonical_name);
	rugged_exception_check(remote_name_size);

	remote_name = xmalloc(remote_name_size * sizeof(char));

	error = git_branch_remote_name(
			remote_name, remote_name_size,
			repo, canonical_name);

	if (error > 0)
		result = rb_enc_str_new(
				remote_name, remote_name_size - 1,
				rb_utf8_encoding());

	xfree(remote_name);
	rugged_exception_check(error);
	return result;
}

/*
 *  call-seq:
 *    branch.remote_name -> string
 *
 *  Get the name of the remote the branch belongs to.
 *
 *  If +branch+ is a remote branch, the name of the remote it belongs to is
 *  returned. If +branch+ is a tracking branch, the name of the remote
 *  of the tracked branch is returned.
 *
 *  Otherwise, +nil+ is returned.
 */
static VALUE rb_git_branch_remote_name(VALUE self)
{
	git_reference *branch, *remote_ref;
	int error = 0;

	Data_Get_Struct(self, git_reference, branch);

	if (git_reference_is_remote(branch)) {
		remote_ref = branch;
	} else {
		error = git_branch_upstream(&remote_ref, branch);

		if (error == GIT_ENOTFOUND)
			return Qnil;

		rugged_exception_check(error);
	}

	return rb_git_branch__remote_name(
			rugged_owner(self),
			git_reference_name(remote_ref));
}

/*
 *  call-seq:
 *    branch.upstream -> branch
 *
 *  Returns the remote tracking branch, or +nil+ if the branch is
 *  remote or has no tracking branch.
 */
static VALUE rb_git_branch_upstream(VALUE self)
{
	git_reference *branch, *upstream_branch;
	int error;

	Data_Get_Struct(self, git_reference, branch);

	if (git_reference_is_remote(branch))
		return Qnil;

	error = git_branch_upstream(&upstream_branch, branch);

	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	return rugged_branch_new(rugged_owner(self), upstream_branch);
}

/*
 *  call-seq:
 *    branch.upstream = branch
 *
 *  Set the upstream configuration for a given local branch.
 *
 *  Takes a local or remote Rugged::Branch instance or a Rugged::Reference
 *  pointing to a branch.
 */
static VALUE rb_git_branch_set_upstream(VALUE self, VALUE rb_branch)
{
	git_reference *branch, *target_branch;
	const char *target_branch_name;

	Data_Get_Struct(self, git_reference, branch);
	if (!NIL_P(rb_branch)) {
		if (!rb_obj_is_kind_of(rb_branch, rb_cRuggedReference))
			rb_raise(rb_eTypeError, "Expecting a Rugged::Reference instance");

		Data_Get_Struct(rb_branch, git_reference, target_branch);

		rugged_exception_check(
			git_branch_name(&target_branch_name, target_branch)
		);
	} else {
		target_branch_name = NULL;
	}

	rugged_exception_check(
		git_branch_set_upstream(branch, target_branch_name)
	);

	return rb_branch;
}

void Init_rugged_branch(void)
{
	rb_cRuggedBranch = rb_define_class_under(rb_mRugged, "Branch", rb_cRuggedReference);

	rb_define_singleton_method(rb_cRuggedBranch, "lookup", rb_git_branch_lookup, -1);

	rb_define_method(rb_cRuggedBranch, "head?", rb_git_branch_head_p, 0);
	rb_define_method(rb_cRuggedBranch, "name", rb_git_branch_name, 0);
	rb_define_method(rb_cRuggedBranch, "remote_name", rb_git_branch_remote_name, 0);
	rb_define_method(rb_cRuggedBranch, "upstream", rb_git_branch_upstream, 0);
	rb_define_method(rb_cRuggedBranch, "upstream=", rb_git_branch_set_upstream, 1);
}
