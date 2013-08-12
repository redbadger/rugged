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
VALUE rb_cRuggedReference;

void rb_git_ref__free(git_reference *ref)
{
	git_reference_free(ref);
}

VALUE rugged_ref_new(VALUE klass, VALUE owner, git_reference *ref)
{
	VALUE rb_ref = Data_Wrap_Struct(klass, NULL, &rb_git_ref__free, ref);
	rugged_set_owner(rb_ref, owner);
	return rb_ref;
}

/*
 *  call-seq:
 *    ref.peel -> oid
 *
 *  Peels tag objects to the sha that they point at. Replicates
 *  +git show-ref --dereference+.
 */
static VALUE rb_git_ref_peel(VALUE self)
{
	/* Leave room for \0 */
	git_reference *ref;
	git_object *object;
	char oid[GIT_OID_HEXSZ + 1];
	int error;

	Data_Get_Struct(self, git_reference, ref);

	error = git_reference_peel(&object, ref, GIT_OBJ_ANY);
	if (error == GIT_ENOTFOUND)
		return Qnil;
	else
		rugged_exception_check(error);

	if (git_reference_type(ref) == GIT_REF_OID &&
			!git_oid_cmp(git_object_id(object), git_reference_target(ref))) {
		git_object_free(object);
		return Qnil;
	} else {
		git_oid_tostr(oid, sizeof(oid), git_object_id(object));
		git_object_free(object);
		return rb_str_new_utf8(oid);
	}
}

/*
 *  call-seq:
 *    reference.target -> oid
 *    reference.target -> ref_name
 *
 *  Return the target of the reference, which is an OID for +:direct+
 *  references, and the name of another reference for +:symbolic+ ones.
 *
 *    r1.type #=> :symbolic
 *    r1.target #=> "refs/heads/master"
 *
 *    r2.type #=> :direct
 *    r2.target #=> "de5ba987198bcf2518885f0fc1350e5172cded78"
 */
static VALUE rb_git_ref_target(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);

	if (git_reference_type(ref) == GIT_REF_OID) {
		return rugged_create_oid(git_reference_target(ref));
	} else {
		return rb_str_new_utf8(git_reference_symbolic_target(ref));
	}
}

/*
 *  call-seq:
 *    reference.type -> :symbolic or :direct
 *
 *  Return whether the reference is +:symbolic+ or +:direct+
 */
static VALUE rb_git_ref_type(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);

	switch (git_reference_type(ref)) {
		case GIT_REF_OID:
			return CSTR2SYM("direct");
		case GIT_REF_SYMBOLIC:
			return CSTR2SYM("symbolic");
		default:
			return Qnil;
	}
}

/*
 *  call-seq:
 *    reference.name -> name
 *
 *  Returns the name of the reference
 *
 *    reference.name #=> 'HEAD'
 */
static VALUE rb_git_ref_name(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);
	return rb_str_new_utf8(git_reference_name(ref));
}

/*
 *  call-seq:
 *    reference.resolve -> peeled_ref
 *
 *  Peel a symbolic reference to its target reference.
 *
 *    r1.type #=> :symbolic
 *    r1.name #=> 'HEAD'
 *    r1.target #=> 'refs/heads/master'
 *
 *    r2 = r1.resolve #=> #<Rugged::Reference:0x401b3948>
 *    r2.target #=> '9d09060c850defbc7711d08b57def0d14e742f4e'
 */
static VALUE rb_git_ref_resolve(VALUE self)
{
	git_reference *ref;
	git_reference *resolved;
	int error;

	Data_Get_Struct(self, git_reference, ref);

	error = git_reference_resolve(&resolved, ref);
	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, rugged_owner(self), resolved);
}

static VALUE reflog_entry_new(const git_reflog_entry *entry)
{
	VALUE rb_entry = rb_hash_new();
	const char *message;

	rb_hash_aset(rb_entry,
		CSTR2SYM("id_old"),
		rugged_create_oid(git_reflog_entry_id_old(entry))
	);

	rb_hash_aset(rb_entry,
		CSTR2SYM("id_new"),
		rugged_create_oid(git_reflog_entry_id_new(entry))
	);

	rb_hash_aset(rb_entry,
		CSTR2SYM("committer"),
		rugged_signature_new(git_reflog_entry_committer(entry), NULL)
	);

	if ((message = git_reflog_entry_message(entry)) != NULL) {
		rb_hash_aset(rb_entry, CSTR2SYM("message"), rb_str_new_utf8(message));
	}

	return rb_entry;
}

/*
 *  call-seq:
 *    reference.log -> [reflog_entry, ...]
 *
 *  Return an array with the log of all modifications to this reference
 *
 *  Each +reflog_entry+ is a hash with the following keys:
 *
 *  - +:id_old+: previous OID before the change
 *  - +:id_new+: OID after the change
 *  - +:committer+: author of the change
 *  - +:message+: message for the change
 *
 *  Example:
 *
 *    reference.log #=> [
 *    # {
 *    #  :id_old => nil,
 *    #  :id_new => '9d09060c850defbc7711d08b57def0d14e742f4e',
 *    #  :committer => {:name => 'Vicent Marti', :email => {'vicent@github.com'}},
 *    #  :message => 'created reference'
 *    # }, ... ]
 */
static VALUE rb_git_reflog(VALUE self)
{
	git_reflog *reflog;
	git_reference *ref;
	int error;
	VALUE rb_log;
	size_t i, ref_count;

	Data_Get_Struct(self, git_reference, ref);

	error = git_reflog_read(&reflog, ref);
	rugged_exception_check(error);

	ref_count = git_reflog_entrycount(reflog);
	rb_log = rb_ary_new2(ref_count);

	for (i = 0; i < ref_count; ++i) {
		const git_reflog_entry *entry =
			git_reflog_entry_byindex(reflog, ref_count - i - 1);

		rb_ary_push(rb_log, reflog_entry_new(entry));
	}

	git_reflog_free(reflog);
	return rb_log;
}

/*
 *  call-seq:
 *    reference.log? -> true or false
 *
 *  Return +true+ if the reference has a reflog, +false+ otherwise.
 */
static VALUE rb_git_has_reflog(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);
	return git_reference_has_log(ref) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *    reference.log!(committer, message = nil) -> nil
 *
 *  Log a modification for this reference to the reflog.
 */
static VALUE rb_git_reflog_write(int argc, VALUE *argv, VALUE self)
{
	git_reference *ref;
	git_reflog *reflog;
	int error;

	VALUE rb_committer, rb_message;

	git_signature *committer;
	const char *message = NULL;

	Data_Get_Struct(self, git_reference, ref);

	rb_scan_args(argc, argv, "11", &rb_committer, &rb_message);

	if (!NIL_P(rb_message)) {
		Check_Type(rb_message, T_STRING);
		message = StringValueCStr(rb_message);
	}

	error = git_reflog_read(&reflog, ref);
	rugged_exception_check(error);

	committer = rugged_signature_get(rb_committer);

	if (!(error = git_reflog_append(reflog,
					git_reference_target(ref),
					committer,
					message)))
		error = git_reflog_write(reflog);

	git_reflog_free(reflog);
	git_signature_free(committer);

	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    reference.branch? -> true or false
 *
 *  Return whether a given reference is a branch
 */
static VALUE rb_git_ref_is_branch(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);
	return git_reference_is_branch(ref) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *    reference.remote? -> true or false
 *
 *  Return whether a given reference is a remote
 */
static VALUE rb_git_ref_is_remote(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);
	return git_reference_is_remote(ref) ? Qtrue : Qfalse;
}

void Init_rugged_reference(void)
{
	rb_cRuggedReference = rb_define_class_under(rb_mRugged, "Reference", rb_cObject);

	rb_define_method(rb_cRuggedReference, "target", rb_git_ref_target, 0);
	rb_define_method(rb_cRuggedReference, "peel", rb_git_ref_peel, 0);

	rb_define_method(rb_cRuggedReference, "type", rb_git_ref_type, 0);

	rb_define_method(rb_cRuggedReference, "name", rb_git_ref_name, 0);

	rb_define_method(rb_cRuggedReference, "resolve", rb_git_ref_resolve, 0);

	rb_define_method(rb_cRuggedReference, "branch?", rb_git_ref_is_branch, 0);
	rb_define_method(rb_cRuggedReference, "remote?", rb_git_ref_is_remote, 0);

	rb_define_method(rb_cRuggedReference, "log", rb_git_reflog, 0);
	rb_define_method(rb_cRuggedReference, "log?", rb_git_has_reflog, 0);
	rb_define_method(rb_cRuggedReference, "log!", rb_git_reflog_write, -1);
}
