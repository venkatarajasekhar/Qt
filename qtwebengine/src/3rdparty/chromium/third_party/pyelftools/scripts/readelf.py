#!/usr/bin/env python
#-------------------------------------------------------------------------------
# scripts/readelf.py
#
# A clone of 'readelf' in Python, based on the pyelftools library
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
import os, sys
from optparse import OptionParser
import string

# If elftools is not installed, maybe we're running from the root or scripts
# dir of the source distribution
try:
    import elftools
except ImportError:
    sys.path.extend(['.', '..'])

from elftools import __version__
from elftools.common.exceptions import ELFError
from elftools.common.py3compat import (
        ifilter, byte2int, bytes2str, itervalues, str2bytes)
from elftools.elf.elffile import ELFFile
from elftools.elf.segments import InterpSegment
from elftools.elf.sections import SymbolTableSection
from elftools.elf.relocation import RelocationSection
from elftools.elf.descriptions import (
    describe_ei_class, describe_ei_data, describe_ei_version,
    describe_ei_osabi, describe_e_type, describe_e_machine,
    describe_e_version_numeric, describe_p_type, describe_p_flags,
    describe_sh_type, describe_sh_flags,
    describe_symbol_type, describe_symbol_bind, describe_symbol_visibility,
    describe_symbol_shndx, describe_reloc_type,
    )
from elftools.dwarf.dwarfinfo import DWARFInfo
from elftools.dwarf.descriptions import (
    describe_reg_name, describe_attr_value, set_global_machine_arch,
    describe_CFI_instructions, describe_CFI_register_rule,
    describe_CFI_CFA_rule,
    )
from elftools.dwarf.constants import (
    DW_LNS_copy, DW_LNS_set_file, DW_LNE_define_file)
from elftools.dwarf.callframe import CIE, FDE


class ReadElf(object):
    """ display_* methods are used to emit output into the output stream
    """
    def __init__(self, file, output):
        """ file: 
                stream object with the ELF file to read
            
            output:
                output stream to write to
        """
        self.elffile = ELFFile(file)
        self.output = output
        
        # Lazily initialized if a debug dump is requested
        self._dwarfinfo = None

    def display_file_header(self):
        """ Display the ELF file header
        """
        self._emitline('ELF Header:')
        self._emit('  Magic:   ')
        self._emitline(' '.join('%2.2x' % byte2int(b)
                                    for b in self.elffile.e_ident_raw))
        header = self.elffile.header
        e_ident = header['e_ident']
        self._emitline('  Class:                             %s' % 
                describe_ei_class(e_ident['EI_CLASS']))
        self._emitline('  Data:                              %s' % 
                describe_ei_data(e_ident['EI_DATA']))
        self._emitline('  Version:                           %s' % 
                describe_ei_version(e_ident['EI_VERSION']))
        self._emitline('  OS/ABI:                            %s' %
                describe_ei_osabi(e_ident['EI_OSABI']))
        self._emitline('  ABI Version:                       %d' % 
                e_ident['EI_ABIVERSION'])
        self._emitline('  Type:                              %s' %
                describe_e_type(header['e_type']))
        self._emitline('  Machine:                           %s' % 
                describe_e_machine(header['e_machine']))
        self._emitline('  Version:                           %s' %
                describe_e_version_numeric(header['e_version']))
        self._emitline('  Entry point address:               %s' % 
                self._format_hex(header['e_entry']))
        self._emit('  Start of program headers:          %s' % 
                header['e_phoff'])
        self._emitline(' (bytes into file)')
        self._emit('  Start of section headers:          %s' % 
                header['e_shoff'])
        self._emitline(' (bytes into file)')
        self._emitline('  Flags:                             %s' % 
                self._format_hex(header['e_flags']))
        self._emitline('  Size of this header:               %s (bytes)' %
                header['e_ehsize'])
        self._emitline('  Size of program headers:           %s (bytes)' %
                header['e_phentsize'])
        self._emitline('  Number of program headers:         %s' %
                header['e_phnum'])
        self._emitline('  Size of section headers:           %s (bytes)' %
                header['e_shentsize'])
        self._emitline('  Number of section headers:         %s' %
                header['e_shnum'])
        self._emitline('  Section header string table index: %s' %
                header['e_shstrndx'])

    def display_program_headers(self, show_heading=True):
        """ Display the ELF program headers.
            If show_heading is True, displays the heading for this information
            (Elf file type is...)
        """
        self._emitline()
        if self.elffile.num_segments() == 0:
            self._emitline('There are no program headers in this file.')
            return

        elfheader = self.elffile.header
        if show_heading:
            self._emitline('Elf file type is %s' %
                describe_e_type(elfheader['e_type']))
            self._emitline('Entry point is %s' %
                self._format_hex(elfheader['e_entry']))
            # readelf weirness - why isn't e_phoff printed as hex? (for section
            # headers, it is...)
            self._emitline('There are %s program headers, starting at offset %s' % (
                elfheader['e_phnum'], elfheader['e_phoff']))
            self._emitline()

        self._emitline('Program Headers:')

        # Now comes the table of program headers with their attributes. Note
        # that due to different formatting constraints of 32-bit and 64-bit
        # addresses, there are some conditions on elfclass here.
        #
        # First comes the table heading
        #
        if self.elffile.elfclass == 32:
            self._emitline('  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align')
        else:
            self._emitline('  Type           Offset             VirtAddr           PhysAddr')
            self._emitline('                 FileSiz            MemSiz              Flags  Align')

        # Now the entries
        #
        for segment in self.elffile.iter_segments():
            self._emit('  %-14s ' % describe_p_type(segment['p_type']))

            if self.elffile.elfclass == 32:
                self._emitline('%s %s %s %s %s %-3s %s' % (
                    self._format_hex(segment['p_offset'], fieldsize=6),
                    self._format_hex(segment['p_vaddr'], fullhex=True),
                    self._format_hex(segment['p_paddr'], fullhex=True),
                    self._format_hex(segment['p_filesz'], fieldsize=5),
                    self._format_hex(segment['p_memsz'], fieldsize=5),
                    describe_p_flags(segment['p_flags']),
                    self._format_hex(segment['p_align'])))
            else: # 64
                self._emitline('%s %s %s' % (
                    self._format_hex(segment['p_offset'], fullhex=True),
                    self._format_hex(segment['p_vaddr'], fullhex=True),
                    self._format_hex(segment['p_paddr'], fullhex=True)))
                self._emitline('                 %s %s  %-3s    %s' % (
                    self._format_hex(segment['p_filesz'], fullhex=True),
                    self._format_hex(segment['p_memsz'], fullhex=True),
                    describe_p_flags(segment['p_flags']),
                    # lead0x set to False for p_align, to mimic readelf.
                    # No idea why the difference from 32-bit mode :-|
                    self._format_hex(segment['p_align'], lead0x=False)))

            if isinstance(segment, InterpSegment):
                self._emitline('      [Requesting program interpreter: %s]' % 
                    bytes2str(segment.get_interp_name()))

        # Sections to segments mapping
        #
        if self.elffile.num_sections() == 0:
            # No sections? We're done
            return 

        self._emitline('\n Section to Segment mapping:')
        self._emitline('  Segment Sections...')

        for nseg, segment in enumerate(self.elffile.iter_segments()):
            self._emit('   %2.2d     ' % nseg)

            for section in self.elffile.iter_sections():
                if (    not section.is_null() and 
                        segment.section_in_segment(section)):
                    self._emit('%s ' % bytes2str(section.name))

            self._emitline('')

    def display_section_headers(self, show_heading=True):
        """ Display the ELF section headers
        """
        elfheader = self.elffile.header
        if show_heading:
            self._emitline('There are %s section headers, starting at offset %s' % (
                elfheader['e_shnum'], self._format_hex(elfheader['e_shoff'])))

        self._emitline('\nSection Header%s:' % (
            's' if elfheader['e_shnum'] > 1 else ''))

        # Different formatting constraints of 32-bit and 64-bit addresses
        #
        if self.elffile.elfclass == 32:
            self._emitline('  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al')
        else:
            self._emitline('  [Nr] Name              Type             Address           Offset')
            self._emitline('       Size              EntSize          Flags  Link  Info  Align')

        # Now the entries
        #
        for nsec, section in enumerate(self.elffile.iter_sections()):
            self._emit('  [%2u] %-17.17s %-15.15s ' % (
                nsec, bytes2str(section.name), describe_sh_type(section['sh_type'])))

            if self.elffile.elfclass == 32:
                self._emitline('%s %s %s %s %3s %2s %3s %2s' % (
                    self._format_hex(section['sh_addr'], fieldsize=8, lead0x=False),
                    self._format_hex(section['sh_offset'], fieldsize=6, lead0x=False),
                    self._format_hex(section['sh_size'], fieldsize=6, lead0x=False),
                    self._format_hex(section['sh_entsize'], fieldsize=2, lead0x=False),
                    describe_sh_flags(section['sh_flags']),
                    section['sh_link'], section['sh_info'],
                    section['sh_addralign']))
            else: # 64
                self._emitline(' %s  %s' % (
                    self._format_hex(section['sh_addr'], fullhex=True, lead0x=False),
                    self._format_hex(section['sh_offset'],
                        fieldsize=16 if section['sh_offset'] > 0xffffffff else 8,
                        lead0x=False)))
                self._emitline('       %s  %s %3s      %2s   %3s     %s' % (
                    self._format_hex(section['sh_size'], fullhex=True, lead0x=False),
                    self._format_hex(section['sh_entsize'], fullhex=True, lead0x=False),
                    describe_sh_flags(section['sh_flags']),
                    section['sh_link'], section['sh_info'],
                    section['sh_addralign']))

        self._emitline('Key to Flags:')
        self._emit('  W (write), A (alloc), X (execute), M (merge), S (strings)')
        if self.elffile['e_machine'] in ('EM_X86_64', 'EM_L10M'):
            self._emitline(', l (large)')
        else:
            self._emitline()
        self._emitline('  I (info), L (link order), G (group), T (TLS), E (exclude), x (unknown)')
        self._emitline('  O (extra OS processing required) o (OS specific), p (processor specific)')

    def display_symbol_tables(self):
        """ Display the symbol tables contained in the file
        """
        for section in self.elffile.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue

            if section['sh_entsize'] == 0:
                self._emitline("\nSymbol table '%s' has a sh_entsize of zero!" % (
                    bytes2str(section.name)))
                continue

            self._emitline("\nSymbol table '%s' contains %s entries:" % (
                bytes2str(section.name), section.num_symbols()))

            if self.elffile.elfclass == 32:
                self._emitline('   Num:    Value  Size Type    Bind   Vis      Ndx Name')
            else: # 64
                self._emitline('   Num:    Value          Size Type    Bind   Vis      Ndx Name')

            for nsym, symbol in enumerate(section.iter_symbols()):
                # symbol names are truncated to 25 chars, similarly to readelf
                self._emitline('%6d: %s %5d %-7s %-6s %-7s %4s %.25s' % (
                    nsym,
                    self._format_hex(symbol['st_value'], fullhex=True, lead0x=False),
                    symbol['st_size'],
                    describe_symbol_type(symbol['st_info']['type']),
                    describe_symbol_bind(symbol['st_info']['bind']),
                    describe_symbol_visibility(symbol['st_other']['visibility']),
                    describe_symbol_shndx(symbol['st_shndx']),
                    bytes2str(symbol.name)))
        
    def display_relocations(self):
        """ Display the relocations contained in the file
        """
        has_relocation_sections = False
        for section in self.elffile.iter_sections():
            if not isinstance(section, RelocationSection):
                continue

            has_relocation_sections = True
            self._emitline("\nRelocation section '%s' at offset %s contains %s entries:" % (
                bytes2str(section.name),
                self._format_hex(section['sh_offset']),
                section.num_relocations()))
            if section.is_RELA():
                self._emitline("  Offset          Info           Type           Sym. Value    Sym. Name + Addend")
            else:
                self._emitline(" Offset     Info    Type            Sym.Value  Sym. Name")

            # The symbol table section pointed to in sh_link
            symtable = self.elffile.get_section(section['sh_link'])

            for rel in section.iter_relocations():
                hexwidth = 8 if self.elffile.elfclass == 32 else 12
                self._emit('%s  %s %-17.17s' % (
                    self._format_hex(rel['r_offset'], 
                        fieldsize=hexwidth, lead0x=False),
                    self._format_hex(rel['r_info'], 
                        fieldsize=hexwidth, lead0x=False),
                    describe_reloc_type(
                        rel['r_info_type'], self.elffile)))

                if rel['r_info_sym'] == 0:
                    self._emitline()
                    continue

                symbol = symtable.get_symbol(rel['r_info_sym'])
                # Some symbols have zero 'st_name', so instead what's used is
                # the name of the section they point at
                if symbol['st_name'] == 0:
                    symsec = self.elffile.get_section(symbol['st_shndx'])
                    symbol_name = symsec.name
                else:
                    symbol_name = symbol.name
                self._emit(' %s %s%22.22s' % (
                    self._format_hex(
                        symbol['st_value'],
                        fullhex=True, lead0x=False),
                    '  ' if self.elffile.elfclass == 32 else '',
                    bytes2str(symbol_name)))
                if section.is_RELA():
                    self._emit(' %s %x' % (
                        '+' if rel['r_addend'] >= 0 else '-',
                        abs(rel['r_addend'])))
                self._emitline()

        if not has_relocation_sections:
            self._emitline('\nThere are no relocations in this file.')
        
    def display_hex_dump(self, section_spec):
        """ Display a hex dump of a section. section_spec is either a section
            number or a name.
        """
        section = self._section_from_spec(section_spec)
        if section is None:
            self._emitline("Section '%s' does not exist in the file!" % (
                section_spec))
            return

        self._emitline("\nHex dump of section '%s':" % bytes2str(section.name))
        self._note_relocs_for_section(section)
        addr = section['sh_addr']
        data = section.data()
        dataptr = 0

        while dataptr < len(data):
            bytesleft = len(data) - dataptr
            # chunks of 16 bytes per line
            linebytes = 16 if bytesleft > 16 else bytesleft

            self._emit('  %s ' % self._format_hex(addr, fieldsize=8))
            for i in range(16):
                if i < linebytes:
                    self._emit('%2.2x' % byte2int(data[dataptr + i]))
                else:
                    self._emit('  ')
                if i % 4 == 3:
                    self._emit(' ')

            for i in range(linebytes):
                c = data[dataptr + i : dataptr + i + 1]
                if byte2int(c[0]) >= 32 and byte2int(c[0]) < 0x7f:
                    self._emit(bytes2str(c))
                else:
                    self._emit(bytes2str(b'.'))

            self._emitline()
            addr += linebytes
            dataptr += linebytes

        self._emitline()

    def display_string_dump(self, section_spec):
        """ Display a strings dump of a section. section_spec is either a
            section number or a name.
        """
        section = self._section_from_spec(section_spec)
        if section is None:
            self._emitline("Section '%s' does not exist in the file!" % (
                section_spec))
            return

        self._emitline("\nString dump of section '%s':" % bytes2str(section.name))

        found = False
        data = section.data()
        dataptr = 0

        while dataptr < len(data):
            while ( dataptr < len(data) and 
                    not (32 <= byte2int(data[dataptr]) <= 127)):
                dataptr += 1

            if dataptr >= len(data):
                break

            endptr = dataptr
            while endptr < len(data) and byte2int(data[endptr]) != 0:
                endptr += 1

            found = True
            self._emitline('  [%6x]  %s' % (
                dataptr, bytes2str(data[dataptr:endptr])))

            dataptr = endptr

        if not found:
            self._emitline('  No strings found in this section.')
        else:
            self._emitline()

    def display_debug_dump(self, dump_what):
        """ Dump a DWARF section
        """
        self._init_dwarfinfo()
        if self._dwarfinfo is None:
            return
        
        set_global_machine_arch(self.elffile.get_machine_arch())

        if dump_what == 'info':
            self._dump_debug_info()
        elif dump_what == 'decodedline':
            self._dump_debug_line_programs()
        elif dump_what == 'frames':
            self._dump_debug_frames()
        elif dump_what == 'frames-interp':
            self._dump_debug_frames_interp()
        else:
            self._emitline('debug dump not yet supported for "%s"' % dump_what)

    def _format_hex(self, addr, fieldsize=None, fullhex=False, lead0x=True):
        """ Format an address into a hexadecimal string.

            fieldsize:
                Size of the hexadecimal field (with leading zeros to fit the
                address into. For example with fieldsize=8, the format will
                be %08x
                If None, the minimal required field size will be used.

            fullhex:
                If True, override fieldsize to set it to the maximal size 
                needed for the elfclass

            lead0x:
                If True, leading 0x is added
        """
        s = '0x' if lead0x else ''
        if fullhex:
            fieldsize = 8 if self.elffile.elfclass == 32 else 16
        if fieldsize is None:
            field = '%x'
        else:
            field = '%' + '0%sx' % fieldsize
        return s + field % addr
        
    def _section_from_spec(self, spec):
        """ Retrieve a section given a "spec" (either number or name).
            Return None if no such section exists in the file.
        """
        try:
            num = int(spec)
            if num < self.elffile.num_sections():
                return self.elffile.get_section(num) 
            else:
                return None
        except ValueError:
            # Not a number. Must be a name then
            return self.elffile.get_section_by_name(str2bytes(spec))

    def _note_relocs_for_section(self, section):
        """ If there are relocation sections pointing to the givne section,
            emit a note about it.
        """
        for relsec in self.elffile.iter_sections():
            if isinstance(relsec, RelocationSection):
                info_idx = relsec['sh_info']
                if self.elffile.get_section(info_idx) == section:
                    self._emitline('  Note: This section has relocations against it, but these have NOT been applied to this dump.')
                    return

    def _init_dwarfinfo(self):
        """ Initialize the DWARF info contained in the file and assign it to
            self._dwarfinfo.
            Leave self._dwarfinfo at None if no DWARF info was found in the file
        """
        if self._dwarfinfo is not None:
            return
        
        if self.elffile.has_dwarf_info():
            self._dwarfinfo = self.elffile.get_dwarf_info()
        else:
            self._dwarfinfo = None

    def _dump_debug_info(self):
        """ Dump the debugging info section.
        """
        self._emitline('Contents of the .debug_info section:\n')
        
        # Offset of the .debug_info section in the stream
        section_offset = self._dwarfinfo.debug_info_sec.global_offset

        for cu in self._dwarfinfo.iter_CUs():
            self._emitline('  Compilation Unit @ offset %s:' %
                self._format_hex(cu.cu_offset))
            self._emitline('   Length:        %s (%s)' % (
                self._format_hex(cu['unit_length']),
                '%s-bit' % cu.dwarf_format()))
            self._emitline('   Version:       %s' % cu['version']),
            self._emitline('   Abbrev Offset: %s' % cu['debug_abbrev_offset']),
            self._emitline('   Pointer Size:  %s' % cu['address_size'])
            
            # The nesting depth of each DIE within the tree of DIEs must be 
            # displayed. To implement this, a counter is incremented each time
            # the current DIE has children, and decremented when a null die is
            # encountered. Due to the way the DIE tree is serialized, this will
            # correctly reflect the nesting depth
            #
            die_depth = 0
            for die in cu.iter_DIEs():
                if die.is_null():
                    die_depth -= 1
                    continue
                self._emitline(' <%s><%x>: Abbrev Number: %s (%s)' % (
                    die_depth,
                    die.offset,
                    die.abbrev_code,
                    die.tag))
                
                for attr in itervalues(die.attributes):
                    name = attr.name
                    # Unknown attribute values are passed-through as integers
                    if isinstance(name, int):
                        name = 'Unknown AT value: %x' % name
                    self._emitline('    <%2x>   %-18s: %s' % (
                        attr.offset,
                        name,
                        describe_attr_value(
                            attr, die, section_offset)))
                
                if die.has_children:
                    die_depth += 1
                    
        self._emitline()

    def _dump_debug_line_programs(self):
        """ Dump the (decoded) line programs from .debug_line
            The programs are dumped in the order of the CUs they belong to.
        """
        self._emitline('Decoded dump of debug contents of section .debug_line:\n')

        for cu in self._dwarfinfo.iter_CUs():
            lineprogram = self._dwarfinfo.line_program_for_CU(cu)

            cu_filename = ''
            if len(lineprogram['include_directory']) > 0:
                cu_filename = '%s/%s' % (
                    bytes2str(lineprogram['include_directory'][0]),
                    bytes2str(lineprogram['file_entry'][0].name))
            else:
                cu_filename = bytes2str(lineprogram['file_entry'][0].name)

            self._emitline('CU: %s:' % cu_filename)
            self._emitline('File name                            Line number    Starting address')

            # Print each state's file, line and address information. For some
            # instructions other output is needed to be compatible with
            # readelf.
            for entry in lineprogram.get_entries():
                state = entry.state
                if state is None:
                    # Special handling for commands that don't set a new state
                    if entry.command == DW_LNS_set_file:
                        file_entry = lineprogram['file_entry'][entry.args[0] - 1]
                        if file_entry.dir_index == 0:
                            # current directory
                            self._emitline('\n./%s:[++]' % (
                                bytes2str(file_entry.name)))
                        else:
                            self._emitline('\n%s/%s:' % (
                                bytes2str(lineprogram['include_directory'][file_entry.dir_index - 1]),
                                bytes2str(file_entry.name)))
                    elif entry.command == DW_LNE_define_file:
                        self._emitline('%s:' % (
                            bytes2str(lineprogram['include_directory'][entry.args[0].dir_index])))
                elif not state.end_sequence:
                    # readelf doesn't print the state after end_sequence
                    # instructions. I think it's a bug but to be compatible
                    # I don't print them too.
                    self._emitline('%-35s  %11d  %18s' % (
                        bytes2str(lineprogram['file_entry'][state.file - 1].name),
                        state.line,
                        '0' if state.address == 0 else 
                               self._format_hex(state.address)))
                if entry.command == DW_LNS_copy:
                    # Another readelf oddity...
                    self._emitline()

    def _dump_debug_frames(self):
        """ Dump the raw frame information from .debug_frame
        """
        if not self._dwarfinfo.has_CFI():
            return
        self._emitline('Contents of the .debug_frame section:')

        for entry in self._dwarfinfo.CFI_entries():
            if isinstance(entry, CIE):
                self._emitline('\n%08x %08x %08x CIE' % (
                    entry.offset, entry['length'], entry['CIE_id']))
                self._emitline('  Version:               %d' % entry['version'])
                self._emitline('  Augmentation:          "%s"' % bytes2str(entry['augmentation']))
                self._emitline('  Code alignment factor: %u' % entry['code_alignment_factor'])
                self._emitline('  Data alignment factor: %d' % entry['data_alignment_factor'])
                self._emitline('  Return address column: %d' % entry['return_address_register'])
                self._emitline()
            else: # FDE
                self._emitline('\n%08x %08x %08x FDE cie=%08x pc=%08x..%08x' % (
                    entry.offset,
                    entry['length'],
                    entry['CIE_pointer'],
                    entry.cie.offset,
                    entry['initial_location'],
                    entry['initial_location'] + entry['address_range']))

            self._emit(describe_CFI_instructions(entry))
        self._emitline()

    def _dump_debug_frames_interp(self):
        """ Dump the interpreted (decoded) frame information from .debug_frame
        """
        if not self._dwarfinfo.has_CFI():
            return

        self._emitline('Contents of the .debug_frame section:')

        for entry in self._dwarfinfo.CFI_entries():
            if isinstance(entry, CIE):
                self._emitline('\n%08x %08x %08x CIE "%s" cf=%d df=%d ra=%d' % (
                    entry.offset,
                    entry['length'],
                    entry['CIE_id'],
                    bytes2str(entry['augmentation']),
                    entry['code_alignment_factor'],
                    entry['data_alignment_factor'],
                    entry['return_address_register']))
                ra_regnum = entry['return_address_register']
            else: # FDE
                self._emitline('\n%08x %08x %08x FDE cie=%08x pc=%08x..%08x' % (
                    entry.offset,
                    entry['length'],
                    entry['CIE_pointer'],
                    entry.cie.offset,
                    entry['initial_location'],
                    entry['initial_location'] + entry['address_range']))
                ra_regnum = entry.cie['return_address_register']

            # Print the heading row for the decoded table
            self._emit('   LOC')
            self._emit('  ' if entry.structs.address_size == 4 else '          ')
            self._emit(' CFA      ')

            # Decode the table nad look at the registers it describes.
            # We build reg_order here to match readelf's order. In particular,
            # registers are sorted by their number, and the register matching
            # ra_regnum is always listed last with a special heading.
            decoded_table = entry.get_decoded()
            reg_order = sorted(ifilter(
                lambda r: r != ra_regnum, 
                decoded_table.reg_order))

            # Headings for the registers
            for regnum in reg_order:
                self._emit('%-6s' % describe_reg_name(regnum))
            self._emitline('ra      ')
            
            # Now include ra_regnum in reg_order to print its values similarly
            # to the other registers.
            reg_order.append(ra_regnum)
            for line in decoded_table.table:
                self._emit(self._format_hex(
                    line['pc'], fullhex=True, lead0x=False))
                self._emit(' %-9s' % describe_CFI_CFA_rule(line['cfa']))

                for regnum in reg_order:
                    if regnum in line:
                        s = describe_CFI_register_rule(line[regnum])
                    else:
                        s = 'u'
                    self._emit('%-6s' % s)
                self._emitline()
        self._emitline()

    def _emit(self, s=''):
        """ Emit an object to output
        """
        self.output.write(str(s))

    def _emitline(self, s=''):
        """ Emit an object to output, followed by a newline
        """
        self.output.write(str(s) + '\n')


SCRIPT_DESCRIPTION = 'Display information about the contents of ELF format files'
VERSION_STRING = '%%prog: based on pyelftools %s' % __version__


def main(stream=None):
    # parse the command-line arguments and invoke ReadElf
    optparser = OptionParser(
            usage='usage: %prog [options] <elf-file>',
            description=SCRIPT_DESCRIPTION,
            add_help_option=False, # -h is a real option of readelf
            prog='readelf.py',
            version=VERSION_STRING)
    optparser.add_option('-H', '--help',
            action='store_true', dest='help',
            help='Display this information')
    optparser.add_option('-h', '--file-header',
            action='store_true', dest='show_file_header',
            help='Display the ELF file header')
    optparser.add_option('-l', '--program-headers', '--segments',
            action='store_true', dest='show_program_header',
            help='Display the program headers')
    optparser.add_option('-S', '--section-headers', '--sections',
            action='store_true', dest='show_section_header',
            help="Display the sections' headers")
    optparser.add_option('-e', '--headers',
            action='store_true', dest='show_all_headers',
            help='Equivalent to: -h -l -S')
    optparser.add_option('-s', '--symbols', '--syms',
            action='store_true', dest='show_symbols',
            help='Display the symbol table')
    optparser.add_option('-r', '--relocs',
            action='store_true', dest='show_relocs',
            help='Display the relocations (if present)')
    optparser.add_option('-x', '--hex-dump',
            action='store', dest='show_hex_dump', metavar='<number|name>',
            help='Dump the contents of section <number|name> as bytes')
    optparser.add_option('-p', '--string-dump',
            action='store', dest='show_string_dump', metavar='<number|name>',
            help='Dump the contents of section <number|name> as strings')
    optparser.add_option('--debug-dump',
            action='store', dest='debug_dump_what', metavar='<what>',
            help=(
                'Display the contents of DWARF debug sections. <what> can ' +
                'one of {info,decodedline,frames,frames-interp}'))

    options, args = optparser.parse_args()

    if options.help or len(args) == 0:
        optparser.print_help()
        sys.exit(0)

    if options.show_all_headers:
        do_file_header = do_section_header = do_program_header = True
    else:
        do_file_header = options.show_file_header
        do_section_header = options.show_section_header
        do_program_header = options.show_program_header

    with open(args[0], 'rb') as file:
        try:
            readelf = ReadElf(file, stream or sys.stdout)
            if do_file_header:
                readelf.display_file_header()
            if do_section_header:
                readelf.display_section_headers(
                        show_heading=not do_file_header)
            if do_program_header:
                readelf.display_program_headers(
                        show_heading=not do_file_header)
            if options.show_symbols:
                readelf.display_symbol_tables()
            if options.show_relocs:
                readelf.display_relocations()
            if options.show_hex_dump:
                readelf.display_hex_dump(options.show_hex_dump)
            if options.show_string_dump:
                readelf.display_string_dump(options.show_string_dump)
            if options.debug_dump_what:
                readelf.display_debug_dump(options.debug_dump_what)
        except ELFError as ex:
            sys.stderr.write('ELF error: %s\n' % ex)
            sys.exit(1)


def profile_main():
    # Run 'main' redirecting its output to readelfout.txt
    # Saves profiling information in readelf.profile
    PROFFILE = 'readelf.profile'
    import cProfile
    cProfile.run('main(open("readelfout.txt", "w"))', PROFFILE)

    # Dig in some profiling stats
    import pstats
    p = pstats.Stats(PROFFILE)
    p.sort_stats('cumulative').print_stats(25)


#-------------------------------------------------------------------------------
if __name__ == '__main__':
    main()
    #profile_main()


