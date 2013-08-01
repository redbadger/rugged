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
extern VALUE rb_cRuggedReference;
VALUE rb_cRuggedReferenceCollection;

static VALUE rb_git_reference_collection_initialize(VALUE self, VALUE repo) {
	rugged_set_owner(self, repo);
	return self;
}

/*
 *  call-seq:
 *    references.add(name, oid, force = false) -> new_ref
 *    references.add(name, target, force = false) -> new_ref
 *
 *  Create a symbolic or direct reference on +repository+ with the given +name+.
 *  If the third argument is a valid OID, the reference will be created as direct.
 *  Otherwise, it will be assumed the target is the name of another reference.
 *
 *  If a reference with the given +name+ already exists and +force+ is +true+,
 *  it will be overwritten. Otherwise, an exception will be raised.
 */
static VALUE rb_git_reference_collection_add(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_repo = rugged_owner(self), rb_name, rb_target, rb_force;
	git_repository *repo;
	git_reference *ref;
	git_oid oid;
	int error, force = 0;

	rb_scan_args(argc, argv, "21", &rb_name, &rb_target, &rb_force);

	Data_Get_Struct(rb_repo, git_repository, repo);
	Check_Type(rb_name, T_STRING);
	Check_Type(rb_target, T_STRING);

	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	if (git_oid_fromstr(&oid, StringValueCStr(rb_target)) == GIT_OK) {
		error = git_reference_create(
			&ref, repo, StringValueCStr(rb_name), &oid, force);
	} else {
		error = git_reference_symbolic_create(
			&ref, repo, StringValueCStr(rb_name), StringValueCStr(rb_target), force);
	}

	rugged_exception_check(error);
	return rugged_ref_new(rb_cRuggedReference, rb_repo, ref);
}

static VALUE rb_git_reference_collection_aref(VALUE self, VALUE name) {
	VALUE rb_repo = rugged_owner(self);
	git_repository *repo;
	git_reference *ref;
	int error;

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_reference_lookup(&ref, repo, StringValueCStr(name));

	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, rb_repo, ref);
}

void Init_rugged_reference_collection(void)
{
	rb_cRuggedReferenceCollection = rb_define_class_under(rb_mRugged, "ReferenceCollection", rb_cObject);
	rb_include_module(rb_cRuggedReferenceCollection, rb_mEnumerable);

	rb_define_method(rb_cRuggedReferenceCollection, "initialize", rb_git_reference_collection_initialize, 1);
	rb_define_method(rb_cRuggedReferenceCollection, "[]", rb_git_reference_collection_aref, 1);
	rb_define_method(rb_cRuggedReferenceCollection, "add", rb_git_reference_collection_add, -1);
	// rb_define_method(rb_cRuggedReferenceCollection, "rename", rb_git_reference_collection_rename, -1);
	// rb_define_method(rb_cRuggedReferenceCollection, "update", rb_git_reference_collection_update, 2);
	// rb_define_method(rb_cRuggedReferenceCollection, "delete", rb_git_reference_collection_delete, 1);
}
