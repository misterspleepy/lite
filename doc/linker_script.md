## Basic linker scriptconcepts
### Linker script format
test files, series of commands, Each command is either a keyword, or an assignment to a symbol.
Strings such as file or format names can normally be entered directly. you may put the file name in double quotes
comments: /* */
### Simple linker script example
SECTIONS
{
    . = 0x1000;
    .text : {
        *(.text)
    }
    . = 0x80000000;
    .data : { *(.data)}
    .bss : { *(.bss) }
}
SECTIONS command as the keyword, followed by a series of symbol assignments and output section descriptions enclosed in curly braces. The first line in the above example sets the special symbol .; 
### Setting the entry point
The first instruction to execute in a program is called the entry point. You can use the ENTRY linker script command to set the entry point. The argument is a symbol name:
 ENTRY ( symbol )
T
The -e entry command-line option;
The ENTRY command in a linker script
The value of symbol, start, if defined
The address of the first byte of the .text section, if present;
The address 0

Commands dealing with files

INCLUDE filename
    Include the linker script filename at this point. The file will be searched for in the current directory, and in any diectory specified with the -L option. You can nest calls to INCLUDE up to 1- levels deep.
INPUT (file, file, ...)
    The INPUT command directs the linker to include the named fils in the link, as though they wher named on the command line.
GROUP (file, file, ...)
    The GROUP command is like INPUT except that the named files should all be archives, and they are searched repeatedly until no new undefined references are created.
OUTPUT (filename)
    The OUTPUT command names the ouput file, Using OUTPUT(FILENAME) in the linker script is exactly like using -o filename on the command line. If both are used, the command line option takes precedence.
SEARCH_DIR (path)
    The command adds path to the list of paths where ld looks for archive libraries. using this is exactly like using -L path
STARTUP (filename)
    The command is just like the INPUT command. except that filename will become the first input file to be linked, as though it were specified firt on the command line. This may be useful when using a system in which the entry point is always the start of the first file.

### Commands dealing with files
### Commands dealing with object file formats
A couple of linker script commands deal with object file formats.
OUTPUT_FORMAT
    The command names which BFD format to use for the output file.
    -oformat bfdname
    OUTPUT_FORMAT (elf32-bigmips, elf32-bigmips, elf32-littlemips)

TARGET(bfdname)
    The TARGET command names which BFD format to use when reading input files. Int affects subsequent INPUT and GROUP commands.

### Other linker script commands
FORCE_COMMON_ALLOCATION
    This command has the sanme effect as the -d command-line option
OUTPUT_ARCH( bfdarch)
    Specify a particular output machine architecture, bpdarch, The argument is one of the names used by the BFD library.
### Assigning values to symbos
You may assign a value to a symbol in a linker script. this will define the symbol as a global symbol.
### Simple assignments
using ffb C assignment operatiors:
symbol = expression;
symbol += expression;
    The first case will define symbol to the value of expression. 

PROVIDE COMMAND
In some cases, it is desirable for a linker scrpt to define a symbol only if it s referenced and is not defined by any object included in the link. For example, traditional linkers defined the symbol etext. However, ANSI C requires that the user be able to use etext as a function name without encountering an error.The PROVIDE keyword may be used to define a symbol, such as etext, only if it is referenced but not defined. The symbox is PROVIDE( symbol = expression)

SECTIONS command

The command tells the linker how to map input sections into output sections, and how to place the output sections in memory. The format of the SECTIONS command is:
SECTIONS
{
    sections - command
}
An ENTRY command
A Symbol assignment
An output section description
An overlay description

The ENTRY command and symbol assignments are permitted inside the SECTIONS command for convenience in using the location counter in those commands. this can also make the linker script easier to understand because you can use those commands at meaningful points in the layout of the output file.

### Output section description
The full description of an output section looks like this:
    SECTION [ address ] [( type )] : [AT (LMA)]
    {
        output-sections-command
        output-sections-command
    }
The colon and the curly braces are also required. The line breaks and other white space are optional.
    Each output-sections-command may be one of the following:
    A symbol assignment
    An input section description
    Data values to include directly
    A special output section keyword

Output section name
    The name of the output section is section. section must meet the constraints of your output format. In formats which only supprt a limited number of sections, such as a.out, the name must be one of the names supported by the format .text .data .bss. If the output format supprts any number of sections, but with numbers and not names, the name should be supplied as a quoted numeric string.
Output section address
    The address is an expression for the VMA of the output section. If you do not provide address, the linker will set it based on REGION if present, or otherwise based on the current value of the location counter.
    If you provide address, the address of the ouput section will be set to precisely that specification. If you provide neither address nor region, then the address of the output section will be set to the current value of the location counter aligned to the alignment requirements of the output section. The alignment requirement of the output section is the strictest alignment of any input section contained within the ouput section

    .text . : { *(.text) }
    .text : { *(.text) }
    
    The first will set the address of the .text output section to the current value of the location counter. The second will set it to the current value of the location counter aligned to the strictest alignment of a .text input section.
    The address may be an arbitray expression. 
        .text ALIGN(ox10) : { *(.text) }

### Input section description
An input section description consists of a file name optionally followed by a list of section names in parentheses. The file name and the section name may be wildcard patterns, which we describe; see Input section wildcard patterns. The most common input section description is to include all input sectons with a particular name in the output section. For example, to include all input .text sections you would write:
    *(.text)
    *(.text .rdata)
    data.o(.data)

When you use a file name, which does not contain any wild card characters, the linker will first see if you also specified the file name on the linker command line or in an INPUT command. if you did not, the linker will attempt to open the file as an input file, as though it appeared on the command line. Note that this differs from an INPUT command, because the linker will not search for the file in the archive search path.

Input section wildcard patterns

In an input secton description, eigher the file name or the section name or both may be wildcard patterns. The file name of * seen in may examples is a simple wildcard pattern for the file name. the wildcard patterns are like those used by the Unix shell.
    *
        matches any number of characters.
    ?
        matches any single character
    
    [ chars ]
        matches a single instance of any of the chars; the - character may be used to specify a range of charcters, as in [a-z] to match any lower case letter

    \
