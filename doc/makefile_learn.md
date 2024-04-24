# 基本使用

## 定义和使用变量
obj = main.o kbd.o
edit : $(obj)
## .PHONY
.PHONY : clean
clean:
    -rm edit $(obj)

## 包含其他Makefile
include <filenames>
filenames 可以包含路径和通配符
include foo.make *.mk
绝对路径和相对路径，-I 选项，prefix/include .INCLUDE_DIRS
-include <filenames> 表示忽略错误

## * Wildcard
* %, * searches your filesystem for matching filenames
$(wildcard *.c)
% Wildcard
matching mode: it matches one or more characters in a string, stem.
replacing mode: it taks the stem that was matched and replaces that in a string
% is most ofen used in rule definitions

Automatic Variables

$@: the file name of the target of the rule. in a pattern rule that has  mutiple targets, $@ is the name of whichever target caused the rule's recipe to be run

$%:
    the target member name, when the target is an archive member, For example, if the target is foo.a(bar.o) then $% is bar.o and $@ is foo.a 

$?
    the names of all the prequisites that are newer than the target, with spaces between them, if the target does not exist, all prerequisites will be included. For prerequisites which are archive members . only the named member is used
    lib : foo.o bar.o lose.o win.o
        ar r lib $?

$^
    The names of all the prerequisites, with spaces between them. For prerequisites which are archive members, only the named member is used.

$+
    This is like $^, but prerequisites listed more than once are dupliated in the order they were listed in the makefile. this is primarily useful for use in linking commands where it is meaningful to repeat library file names in a particular order.

$|
    The names of all ther order-only prerequisites, with spaces between them.

$*
    The stem with which an implicit rule matches. if the target is dir/a.foo.b and the target pattern is a.%.b, then the stem is dir/foo. The stem is useful for constructing names of related files.

$(@D)
    The directory part of the file name of the target, with the trailing slash removed, if the value of $@ is dir/foo.o then $(@D) is dir. This value is . if $@ does not contain a slash

$(@F)
    The file-within-directory part of the file name of the target. if the value of $@ is dir/foo.o then $(@F) is equivalent to $(notdir $@)

$(*D) $(*F)

$(%D) $(%F)

$(<D) $(<F)

$(^D) $(^F) Lists of the directory parts and the file-within-directory parts of all prerequisites.

$(+D) $(+F)

$(?D) $(?F)

Introduction to Pattern Rules

A pattern rule contains the character % in the target. otherwise, it looks exactly like an ordinary rule. the target is a pattern for matching file names.
the % matches any nonempty substring. while other characters match only themselves.

For example, '%.c' as a pattern matches any file name that ends in '.c'

% in a prerequisite of a pattern rule stands for the sanme stem that was matched by % in the target, in order for the pattern rule to apply, its target pattern must match the file name under consideration and all of its prereuistites must name files that exist or can be made. Thest files become prerequisites of the target.

%.o : %.c; recipe...
specifies how to make a file n.o, with another file n.c as its prerequisite, provided that n.c exists or can be made.


Commands and execution

Each command is run in a new shell; 
```
all:
    cd ..
    echo `pwd

    cd ..; echo `pwd`
```

Default Shell 
SHELL=/bin/bash
cool:
    echo "hello from bash"


Double dollar sign:
    if you want a string to have a dollarsign. you can use $$. This is how to user a shell variable in bash

Error handling with -k -i -
Add -k when running make to continue running even in the face of errors. helpful if you want to see all the erros of Make at once.
Add a - befor a command to suppress the error
Add -i to make to have this happer for every command

one:
    -false
    touch one

Recursive use of make

To recursively call a makefile. use the special $(MAKE) instead of make because it will pass the make flags for you and won't itself be affected by them

```
all:
    mkdir -p subdir
    printf $(new_contents) | sed -e 's/^ //' > subdir/makefile
    cd subdir && $(MAKE)
clean:
    rm -rf subdir
```

Export environments, and recursive make

when Make starts, it automatically creates Make variables out of all the environment variables that are set when it's executed

all:
    echo $$shell_env_val
    echo $(shell_env_var)

The exportg directive takes a variable and sets it the environment for all shell commands in all the recipes:
shell_env_var=Shell env var. created inside of Make
export shell_env_var
all:
    echo $(shell_env_var)
    ech $$shell_env_var

As such, when you run the make command inside of make, you can use the export directive to make it accessible to sub-make commands, in this example, cooly is exported such that the makefile in subdir can use it.

Command line arguments and override

you can override variables that come from the command line by using override.
here we ran make with make options_on=hi
override option_one = did_override
option_two = not_override

Target-specific variables
all: one = cool

Pattern-specific variables
%.c: one = cool

Conditional part of Malefiles

all:
    ifeq ($(foo), ok)
        echo ddd
    else
        echo 222
    endif

all:
ifeeq ($(strip $(fool,)
    echo 11
endif
ifeq ($(null),)
echo 11
endif

Check if a variable is defined

if def foo

endif


ifneq(, $findstring i, $MAKEFLAGS)
endif


String Substitution

$(patsubst pattern, replacement, text)


```makefile
foo := a.o b.o l.a c.o
one := $(patsubst %.o,%.c,$(foo))
# This is a shorthand for the above
two := $(foo:%.o=%.c)
# This is the suffix-only shorthand, and is also equivalent to the above.
three := $(foo:.o=.c)

all:
	echo $(one)
	echo $(two)
	echo $(three)
```

Makefile Cookbook
```Makefile
# Thanks to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
TARGET_EXEC := final_program

BUILD_DIR := ./build
SRC_DIRS := ./src

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)

```