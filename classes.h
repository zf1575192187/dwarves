#ifndef _PAHOLE_CLASSES_H_
#define _PAHOLE_CLASSES_H_ 1
/* 
  Copyright (C) 2006 Mandriva Conectiva S.A.
  Copyright (C) 2006 Arnaldo Carvalho de Melo <acme@mandriva.com>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.
*/


#include <stdint.h>
#include <dwarf.h>
#include <elfutils/libdw.h>

#include "list.h"

struct cus {
	struct list_head cus;
	struct list_head priv_definitions;
	struct list_head priv_fwd_decls;
	struct list_head *definitions;
	struct list_head *fwd_decls;
};

struct cu {
	struct list_head node;
	struct list_head classes;
	struct list_head functions;
	struct list_head variables;
	struct list_head tool_list;	/* To be used by tools such as ctracer */
	const char	 *name;
	uint16_t	 language;
	uint32_t	 id;
	unsigned long	 nr_inline_expansions;
	size_t		 size_inline_expansions;
	uint32_t	 nr_functions_changed;
	uint32_t	 nr_structures_changed;
	size_t		 max_len_changed_item;
	size_t		 function_bytes_added;
	size_t		 function_bytes_removed;
};

struct tag {
	struct list_head node;
	Dwarf_Off	 type;
	Dwarf_Off	 id;
	uint16_t	 tag;
	uint16_t	 decl_line;
	const char	 *decl_file;
};

struct class {
	struct tag	 tag;
	struct cu	 *cu;
	struct list_head members;
	struct list_head node;
	const char	 *name;
	size_t		 size;
	uint16_t	 nr_members;
	uint8_t		 nr_holes;
	uint8_t		 nr_bit_holes;
	uint16_t	 padding;
	uint8_t		 bit_padding;
	uint8_t		 declaration:1;
	uint8_t		 visited:1;
	uint8_t		 fwd_decl_emitted:1;
	uint8_t		 array_dimensions:5;
	uint32_t	 refcnt;
	uint32_t	 *array_nr_entries;
	int32_t		 diff;
	struct class	 *class_to_diff;
};

struct class_member {
	struct tag	 tag;
	char		 *name;
	struct class	 *class;
	uint16_t	 offset;
	uint8_t		 bit_offset;
	uint8_t		 bit_size;
	uint8_t		 bit_hole;	/* If there is a bit hole before the next
					   one (or the end of the struct) */
	uint8_t		 visited:1;
	uint16_t	 hole;		/* If there is a hole before the next
					   one (or the end of the struct) */
};

struct lexblock {
	struct list_head inline_expansions;
	struct list_head labels;
	struct list_head variables;
	uint16_t	 nr_inline_expansions;
	uint16_t	 nr_labels;
	uint16_t	 nr_variables;
	size_t		 size_inline_expansions;
};

struct function {
	struct tag	 tag;
	struct cu	 *cu;
	struct lexblock	 lexblock;
	struct list_head parameters;
	struct list_head tool_node;	/* Node to be used by tools */
	const char	 *name;
	Dwarf_Addr	 low_pc;
	Dwarf_Addr	 high_pc;
	uint16_t	 nr_parameters;
	uint16_t	 inlined;
	uint8_t		 external:1;
	uint8_t		 unspecified_parameters;
	uint32_t	 refcnt;
	int32_t		 diff;
	uint32_t	 cu_total_nr_inline_expansions;
	size_t		 cu_total_size_inline_expansions;
	struct class	 *class_to_diff;
};

struct parameter {
	struct tag	 tag;
	char		 *name;
	struct function	 *function;
};

struct variable {
	struct tag	 tag;
	struct cu	 *cu;
	struct list_head cu_node;
	char		 *name;
	Dwarf_Off	 abstract_origin;
};

struct inline_expansion {
	struct tag	 tag;
	struct function	 *function;
	size_t		 size;
	Dwarf_Addr	 low_pc;
	Dwarf_Addr	 high_pc;
};

struct label {
	struct tag	 tag;
	char		 *name;
	Dwarf_Addr	 low_pc;
};

struct enumerator {
	struct tag	 tag;
	const char	 *name;
	uint32_t	 value;
};

#define DEFAULT_CACHELINE_SIZE 32

extern void class__find_holes(struct class *self);
extern void class__print(const struct class *self,
			 const char *prefix, const char *suffix);
extern void function__print(const struct function *self, const int show_stats,
			    const int show_variables,
			    const int show_inline_expansions);

extern struct cus *cus__new(struct list_head *definitions,
			    struct list_head *fwd_decls);
extern int cus__load(struct cus *self, const char *filename);
extern int cus__load_dir(struct cus *self, const char *dirname,
			 const char *filename_mask, const int recursive);
extern struct cu *cus__find_cu_by_name(const struct cus *self,
				       const char *name);
extern struct function *cus__find_function_by_name(const struct cus *self,
						   const char *name);
extern int cus__emit_function_definitions(struct cus *self,
					  struct function *function);
extern int cus__emit_struct_definitions(struct cus *self, struct class *class,
					const char *prefix,
					const char *suffix);
extern int cus__emit_fwd_decl(struct cus *self, struct class *class);

extern struct class *cu__find_class_by_id(const struct cu *cu,
					  const Dwarf_Off type);
extern struct class *cu__find_class_by_name(const struct cu *cu,
					    const char *name);
extern int	    class__is_struct(const struct class *self,
				     struct class **typedef_alias);
extern struct class *cus__find_class_by_name(const struct cus *self,
					     const char *name);
extern void	    cu__account_inline_expansions(struct cu *self);
extern int	    cu__for_each_class(struct cu *self,
				       int (*iterator)(struct class *class,
						       void *cookie),
				       void *cookie,
				 struct class *(*filter)(struct class *class));
extern int	    cu__for_each_function(struct cu *cu,
					  int (*iterator)(struct function *func,
							  void *cookie),
					  void *cookie,
			struct function *(*filter)(struct function *function,
						   void *cookie));
extern void	    cus__for_each_cu(struct cus *self,
				     int (*iterator)(struct cu *cu,
						     void *cookie),
				     void *cookie,
				     struct cu *(*filter)(struct cu *cu));

extern const struct class_member *
		class__find_bit_hole(const struct class *self,
				     const struct class_member *trailer,
				     const size_t bit_hole_size);

extern struct function *cu__find_function_by_id(const struct cu *self,
						const Dwarf_Off id);
extern struct function *cu__find_function_by_name(const struct cu *cu,
						  const char *name);

static inline size_t function__size(const struct function *self)
{
	return self->high_pc - self->low_pc;
}

static inline int function__declared_inline(const struct function *self)
{
	return (self->inlined == DW_INL_declared_inlined ||
	        self->inlined == DW_INL_declared_not_inlined);
}

static inline int function__inlined(const struct function *self)
{
	return (self->inlined == DW_INL_inlined ||
	        self->inlined == DW_INL_declared_inlined);
}

extern int function__has_parameter_of_type(const struct function *self,
					   const struct class *target);

extern const char *class__name(const struct class *self, char *bf, size_t len);

extern struct class_member *class__find_member_by_name(const struct class *self,
						       const char *name);

extern size_t class_member__names(const struct class *type,
				  const struct class_member *self,
				  char *class_name, size_t class_name_size,
				  char *member_name, size_t member_name_size);
extern size_t cacheline_size;

extern const char *variable__name(const struct variable *self);
extern const char *variable__type_name(const struct variable *self,
				       char *bf, size_t len);

extern const char *dwarf_tag_name(const uint32_t tag);

extern int tag__fwd_decl(const struct cu *cu, const struct tag *tag);

extern size_t parameter__names(const struct parameter *self,
			       char *class_name, size_t class_name_size,
			       char *parameter_name,
			       size_t parameter_name_size);

#endif /* _PAHOLE_CLASSES_H_ */
