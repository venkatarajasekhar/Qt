#!/usr/bin/perl
# Copyright (c) 2001-2009 International Business Machines
# Corporation and others. All Rights Reserved.

####################################################################################
# filterRFC3454.pl:
# This tool filters the RFC-3454 txt file for StringPrep tables and creates a table
# to be used in NamePrepProfile
#
# Author: Ram Viswanadha
#        
####################################################################################

use File::Find;
use File::Basename;
use IO::File;
use Cwd;
use File::Copy;
use Getopt::Long;
use File::Path;
use File::Copy;
use Time::localtime;

$icu_copyright = "#####################################################################\n# Copyright (c) %d, International Business Machines Corporation and\n# others. All Rights Reserved.\n#####################################################################\n\n";
$copyright = "###################\n# This file was generated from RFC 3454 (http://www.ietf.org/rfc/rfc3454.txt)\n# Copyright (C) The Internet Society (2002).  All Rights Reserved. \n###################\n\n";
$warning = "###################\n# WARNING: This table is generated by filterRFC3454.pl tool with\n# options: @ARGV \n###################\n\n";
#run the program)
main();

#---------------------------------------------------------------------
# The main program

sub main(){
  GetOptions(
           "--sourcedir=s" => \$sourceDir,
           "--destdir=s" => \$destDir,
           "--src-filename=s" => \$srcFileName,
           "--dest-filename=s" => \$destFileName,
           "--A1"  => \$a1,
           "--B1"  => \$b1,
           "--B2"  => \$b2,
           "--B3"  => \$b3,
           "--C11" => \$c11,
           "--C12" => \$c12,
           "--C21" => \$c21,
           "--C22" => \$c22,
           "--C3"  => \$c3,
           "--C4"  => \$c4,
           "--C5"  => \$c5,
           "--C6"  => \$c6,
           "--C7"  => \$c7,
           "--C8"  => \$c8,
           "--C9"  => \$c9,
           "--iscsi" => \$writeISCSIProhibitedExtra,
           "--xmpp-node" => \$writeXMPPNodeProhibitedExtra,
           "--sasl" => \$writeSASLMap,
           "--ldap" => \$writeLDAPMap,
           "--normalize" => \$norm,
           "--check-bidi" => \$checkBidi,
           );
  usage() unless defined $sourceDir;
  usage() unless defined $destDir;
  usage() unless defined $srcFileName;
  usage() unless defined $destFileName;

  $infile = $sourceDir."/".$srcFileName;
  $inFH = IO::File->new($infile,"r")
            or die  "could not open the file $infile for reading: $! \n";
  $outfile = $destDir."/".$destFileName;

  unlink($outfile);
  $outFH = IO::File->new($outfile,"a")
            or die  "could not open the file $outfile for writing: $! \n";

  printf $outFH  $icu_copyright, localtime->year()+1900;
  print $outFH  $copyright;
  print $outFH  $warning;

  if(defined $norm) {
      print $outFH "\@normalize;;\n";
  }
  if(defined $checkBidi) {
      print $outFH "\@check-bidi;;\n";
  }
  print $outFH "\n";
  close($outFH);

  if(defined $b2 && defined $b3){
      die "ERROR: --B2 and --B3 are both specified\!\n";
  }

  while(defined ($line=<$inFH>)){
      next unless $line=~ /Start\sTable/;
      if($line =~ /A.1/){
            createUnassignedTable($inFH,$outfile);
      }
      if($line =~ /B.1/ && defined $b1){
            createMapToNothing($inFH,$outfile);
      }
      if($line =~ /B.2/ && defined $b2){
            createCaseMapNorm($inFH,$outfile);
      }
      if($line =~ /B.3/ && defined $b3){
            createCaseMapNoNorm($inFH,$outfile);
      }
      if($line =~ /C.1.1/ && defined $c11 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.1.2/ && defined $c12 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.2.1/ && defined $c21 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.2.2/ && defined $c22 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.3/ && defined $c3 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.4/ && defined $c4 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.5/ && defined $c5 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.6/ && defined $c6 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.7/ && defined $c7 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.8/ && defined $c8 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
      if($line =~ /C.9/ && defined $c9 ){
            createProhibitedTable($inFH,$outfile,$line);
      }
  }
  if( defined $writeISCSIProhibitedExtra){
      create_iSCSIExtraProhibitedTable($inFH, $outfile);
  }
  if( defined $writeXMPPNodeProhitedExtra){
      create_XMPPNodeExtraProhibitedTable($inFH, $outfile);
  }
  if( defined $writeSASLMap){
      create_SASLMapTable($inFH, $outfile);
  }
  if( defined $writeLDAPMap){
      create_LDAPMapTable($inFH, $outfile);
  }
  close($inFH);
}

#-----------------------------------------------------------------------
sub readPrint{
    local ($inFH, $outFH,$comment, $table) = @_;
    $count = 0;
    print $outFH $comment."\n";
    while(defined ($line = <$inFH>)){
        next if $line =~ /Hoffman\s\&\sBlanchet/;  # ignore heading
        next if $line =~ /RFC\s3454/; # ignore heading
        next if $line =~ /\f/;  # ignore form feed
        next if $line eq "\n";  # ignore blank lines
        # break if "End Table" is found
        if( $line =~ /End\sTable/){
            print $outFH "\n# Total code points $count\n\n";
            return;
        }
        if($print==1){
            print $line;
        }
        $line =~ s/-/../;
        $line =~ s/^\s+//;
        if($line =~ /\;/){
        }else{
            $line =~ s/$/;/;
        }
        if($table =~ /A/ ){
            ($code, $noise) = split /;/ , $line;
            $line = $code."; ; UNASSIGNED\n";
        }elsif ( $table =~ /B\.1/ ){
            $line =~ s/Map to nothing/MAP/;
        }elsif ( $table =~ /B\.[23]/ ){
            $line =~ s/Case map/MAP/;
            $line =~ s/Additional folding/MAP/;
        }elsif ( $table =~ /C/ ) {
            ($code, $noise) = split /;/ , $line;   
            $line = $code."; ; PROHIBITED\n";
        }
        if($line =~ /\.\./){
            ($code, $noise) = split /;/ , $line;
            ($startStr, $endStr ) = split /\.\./, $code;
            $start = atoi($startStr);
            $end   = atoi($endStr);
            #print $start."     ".$end."\n";
            while($start <= $end){
                $count++;
                $start++;
            }
        }else{
              $count++;
        }
        print $outFH $line;
    }
}
#-----------------------------------------------------------------------
sub atoi {
    my $t;
    foreach my $d (split(//, shift())) {
        $t = $t * 16 + $d;
    }
    return $t;
}
#-----------------------------------------------------------------------
sub createUnassignedTable{
    ($inFH,$outfile) = @_;
    $outFH = IO::File->new($outfile,"a")
            or die  "could not open the file $outfile for writing: $! \n";
    $comment = "# This table contains code points from Table A.1 from RFC 3454\n";
    readPrint($inFH,$outFH, $comment, "A");
    close($outFH);
}
#-----------------------------------------------------------------------
sub createMapToNothing{
    ($inFH,$outfile) = @_;
    $outFH = IO::File->new($outfile,"a")
            or die  "could not open the file $outfile for writing: $! \n";
    $comment = "# This table contains code points from Table B.1 from RFC 3454\n";
    readPrint($inFH,$outFH,$comment, "B.1");
    close($outFH);
}
#-----------------------------------------------------------------------
sub createCaseMapNorm{
    ($inFH,$outfile) = @_;
    $outFH = IO::File->new($outfile,"a")
            or die  "could not open the file $outfile for writing: $! \n";
    $comment = $warning."# This table contains code points from Table B.2 from RFC 3454\n";
    readPrint($inFH,$outFH,$comment, "B.2");
    close($outFH);
}
#-----------------------------------------------------------------------
sub createCaseMapNoNorm{
    ($inFH,$outfile) = @_;
    $outFH = IO::File->new($outfile,"a")
            or die  "could not open the file $outfile for writing: $! \n";
    $comment = $warning."# This table contains code points from Table B.3 from RFC 3454\n";
    readPrint($inFH,$outFH,$comment, "B.3");
    close($outFH);
}
#-----------------------------------------------------------------------
sub createProhibitedTable{
    ($inFH,$outfile,$line) = @_;
    $line =~ s/Start//;
    $line =~ s/-//g;
    $comment = "# code points from $line";

    $outFH = IO::File->new($outfile, "a")
            or die  "could not open the file $outfile for writing: $! \n";
    readPrint($inFH,$outFH,$comment, "C");
    close($outFH);
}

#-----------------------------------------------------------------------
sub create_iSCSIExtraProhibitedTable{
    ($inFH,$outfile,$line) = @_;
    $comment ="# Additional prohibitions from iSCSI profile (rfc3722.txt)\n\n";

    $outFH = IO::File->new($outfile, "a")
            or die  "could not open the file $outfile for writing: $! \n";
    print $outFH $comment;
    print $outFH "0021..002C; ; PROHIBITED\n";
    print $outFH "002F; ; PROHIBITED\n";
    print $outFH "003B..0040; ; PROHIBITED\n";
    print $outFH "005B..0060; ; PROHIBITED\n";
    print $outFH "007B..007E; ; PROHIBITED\n";
    print $outFH "3002; ; PROHIBITED\n";
    print $outFH "\n# Total code points 30\n";
    close($outFH);
}
#-----------------------------------------------------------------------
sub create_XMPPNodeExtraProhibitedTable{
    ($inFH,$outfile,$line) = @_;
    $comment ="# Additional prohibitions from XMPP Nodeprep profile (rfc3920.txt)\n\n";

    $outFH = IO::File->new($outfile, "a")
            or die  "could not open the file $outfile for writing: $! \n";
    print $outFH $comment;
    print $outFH "0022; ; PROHIBITED\n";
    print $outFH "0026; ; PROHIBITED\n";
    print $outFH "0027; ; PROHIBITED\n";
    print $outFH "002F; ; PROHIBITED\n";
    print $outFH "003A; ; PROHIBITED\n";
    print $outFH "003C; ; PROHIBITED\n";
    print $outFH "003E; ; PROHIBITED\n";
    print $outFH "0040; ; PROHIBITED\n";
    print $outFH "\n# Total code points 8\n";
    close($outFH);
}
#-----------------------------------------------------------------------
sub create_SASLMapTable{
    ($inFH,$outfile,$line) = @_;
    $comment ="# Map table for SASL profile (rfc4013.txt)\n\n";

    $outFH = IO::File->new($outfile, "a")
            or die  "could not open the file $outfile for writing: $! \n";
    print $outFH $comment;
    # non-ASCII space characters [C.1.2] to SPACE
    print $outFH "00A0; 0020; MAP\n";
    print $outFH "1680; 0020; MAP\n";
    print $outFH "2000; 0020; MAP\n";
    print $outFH "2001; 0020; MAP\n";
    print $outFH "2002; 0020; MAP\n";
    print $outFH "2003; 0020; MAP\n";
    print $outFH "2004; 0020; MAP\n";
    print $outFH "2005; 0020; MAP\n";
    print $outFH "2006; 0020; MAP\n";
    print $outFH "2007; 0020; MAP\n";
    print $outFH "2008; 0020; MAP\n";
    print $outFH "2009; 0020; MAP\n";
    print $outFH "200A; 0020; MAP\n";
    print $outFH "200B; 0020; MAP\n";
    print $outFH "202F; 0020; MAP\n";
    print $outFH "205F; 0020; MAP\n";
    print $outFH "3000; 0020; MAP\n";

    # commonly mapped to nothing characters except U+200B to nothing
    print $outFH "00AD; ; MAP\n";
    print $outFH "034F; ; MAP\n";
    print $outFH "1806; ; MAP\n";
    print $outFH "180B; ; MAP\n";
    print $outFH "180C; ; MAP\n";
    print $outFH "180D; ; MAP\n";
    print $outFH "200C; ; MAP\n";
    print $outFH "200D; ; MAP\n";
    print $outFH "2060; ; MAP\n";
    print $outFH "FE00; ; MAP\n";
    print $outFH "FE01; ; MAP\n";
    print $outFH "FE02; ; MAP\n";
    print $outFH "FE03; ; MAP\n";
    print $outFH "FE04; ; MAP\n";
    print $outFH "FE05; ; MAP\n";
    print $outFH "FE06; ; MAP\n";
    print $outFH "FE07; ; MAP\n";
    print $outFH "FE08; ; MAP\n";
    print $outFH "FE09; ; MAP\n";
    print $outFH "FE0A; ; MAP\n";
    print $outFH "FE0B; ; MAP\n";
    print $outFH "FE0C; ; MAP\n";
    print $outFH "FE0D; ; MAP\n";
    print $outFH "FE0E; ; MAP\n";
    print $outFH "FE0F; ; MAP\n";
    print $outFH "FEFF; ; MAP\n";
    print $outFH "\n# Total code points 43\n";
    close($outFH);
}
#-----------------------------------------------------------------------
sub create_LDAPMapTable{
    ($inFH,$outfile,$line) = @_;
    $comment ="# Map table for LDAP profile (rfc4518.txt)\n\n";

    $outFH = IO::File->new($outfile, "a")
            or die  "could not open the file $outfile for writing: $! \n";
    print $outFH $comment;

    #   SOFT HYPHEN (U+00AD) and MONGOLIAN TODO SOFT HYPHEN (U+1806) code
    #   points are mapped to nothing.  COMBINING GRAPHEME JOINER (U+034F) and
    #   VARIATION SELECTORs (U+180B-180D, FF00-FE0F) code points are also
    #   mapped to nothing.  The OBJECT REPLACEMENT CHARACTER (U+FFFC) is
    #   mapped to nothing.

    print $outFH "00AD; ; MAP\n";
    print $outFH "034F; ; MAP\n";
    print $outFH "1806; ; MAP\n";
    print $outFH "180B; ; MAP\n";
    print $outFH "180C; ; MAP\n";
    print $outFH "180D; ; MAP\n";
    print $outFH "FE00; ; MAP\n";
    print $outFH "FE01; ; MAP\n";
    print $outFH "FE02; ; MAP\n";
    print $outFH "FE03; ; MAP\n";
    print $outFH "FE04; ; MAP\n";
    print $outFH "FE05; ; MAP\n";
    print $outFH "FE06; ; MAP\n";
    print $outFH "FE07; ; MAP\n";
    print $outFH "FE08; ; MAP\n";
    print $outFH "FE09; ; MAP\n";
    print $outFH "FE0A; ; MAP\n";
    print $outFH "FE0B; ; MAP\n";
    print $outFH "FE0C; ; MAP\n";
    print $outFH "FE0D; ; MAP\n";
    print $outFH "FE0E; ; MAP\n";
    print $outFH "FE0F; ; MAP\n";
    print $outFH "FFFC; ; MAP\n";

#   CHARACTER TABULATION (U+0009), LINE FEED (LF) (U+000A), LINE
#   TABULATION (U+000B), FORM FEED (FF) (U+000C), CARRIAGE RETURN (CR)
#   (U+000D), and NEXT LINE (NEL) (U+0085) are mapped to SPACE (U+0020).

    print $outFH "0009; 0020; MAP\n";
    print $outFH "000A; 0020; MAP\n";
    print $outFH "000B; 0020; MAP\n";
    print $outFH "000C; 0020; MAP\n";
    print $outFH "000D; 0020; MAP\n";
    print $outFH "0085; 0020; MAP\n";

    #   All other control code (e.g., Cc) points or code points with a
    #   control function (e.g., Cf) are mapped to nothing.  The following is
    #   a complete list of these code points: U+0000-0008, 000E-001F, 007F-
    #   0084, 0086-009F, 06DD, 070F, 180E, 200C-200F, 202A-202E, 2060-2063,
    #   206A-206F, FEFF, FFF9-FFFB, 1D173-1D17A, E0001, E0020-E007F.

    print $outFH "0000; ; MAP\n";
    print $outFH "0001; ; MAP\n";
    print $outFH "0002; ; MAP\n";
    print $outFH "0003; ; MAP\n";
    print $outFH "0004; ; MAP\n";
    print $outFH "0005; ; MAP\n";
    print $outFH "0006; ; MAP\n";
    print $outFH "0007; ; MAP\n";
    print $outFH "0008; ; MAP\n";
    print $outFH "000E; ; MAP\n";
    print $outFH "000F; ; MAP\n";
    print $outFH "0010; ; MAP\n";
    print $outFH "0011; ; MAP\n";
    print $outFH "0012; ; MAP\n";
    print $outFH "0013; ; MAP\n";
    print $outFH "0014; ; MAP\n";
    print $outFH "0015; ; MAP\n";
    print $outFH "0016; ; MAP\n";
    print $outFH "0017; ; MAP\n";
    print $outFH "0018; ; MAP\n";
    print $outFH "0019; ; MAP\n";
    print $outFH "001A; ; MAP\n";
    print $outFH "001B; ; MAP\n";
    print $outFH "001C; ; MAP\n";
    print $outFH "001D; ; MAP\n";
    print $outFH "001E; ; MAP\n";
    print $outFH "001F; ; MAP\n";
    print $outFH "007F; ; MAP\n";
    print $outFH "0080; ; MAP\n";
    print $outFH "0081; ; MAP\n";
    print $outFH "0082; ; MAP\n";
    print $outFH "0083; ; MAP\n";
    print $outFH "0084; ; MAP\n";
    print $outFH "0086; ; MAP\n";
    print $outFH "0087; ; MAP\n";
    print $outFH "0088; ; MAP\n";
    print $outFH "0089; ; MAP\n";
    print $outFH "008A; ; MAP\n";
    print $outFH "008B; ; MAP\n";
    print $outFH "008C; ; MAP\n";
    print $outFH "008D; ; MAP\n";
    print $outFH "008E; ; MAP\n";
    print $outFH "008F; ; MAP\n";
    print $outFH "0090; ; MAP\n";
    print $outFH "0091; ; MAP\n";
    print $outFH "0092; ; MAP\n";
    print $outFH "0093; ; MAP\n";
    print $outFH "0094; ; MAP\n";
    print $outFH "0095; ; MAP\n";
    print $outFH "0096; ; MAP\n";
    print $outFH "0097; ; MAP\n";
    print $outFH "0098; ; MAP\n";
    print $outFH "0099; ; MAP\n";
    print $outFH "009A; ; MAP\n";
    print $outFH "009B; ; MAP\n";
    print $outFH "009C; ; MAP\n";
    print $outFH "009D; ; MAP\n";
    print $outFH "009E; ; MAP\n";
    print $outFH "009F; ; MAP\n";
    print $outFH "06DD; ; MAP\n";
    print $outFH "070F; ; MAP\n";
    print $outFH "180E; ; MAP\n";
    print $outFH "200C; ; MAP\n";
    print $outFH "200D; ; MAP\n";
    print $outFH "200E; ; MAP\n";
    print $outFH "200F; ; MAP\n";
    print $outFH "202A; ; MAP\n";
    print $outFH "202B; ; MAP\n";
    print $outFH "202C; ; MAP\n";
    print $outFH "202D; ; MAP\n";
    print $outFH "202E; ; MAP\n";
    print $outFH "2060; ; MAP\n";
    print $outFH "2061; ; MAP\n";
    print $outFH "2062; ; MAP\n";
    print $outFH "2063; ; MAP\n";
    print $outFH "206A; ; MAP\n";
    print $outFH "206B; ; MAP\n";
    print $outFH "206C; ; MAP\n";
    print $outFH "206D; ; MAP\n";
    print $outFH "206E; ; MAP\n";
    print $outFH "206F; ; MAP\n";
    print $outFH "FEFF; ; MAP\n";
    print $outFH "FFF9; ; MAP\n";
    print $outFH "FFFA; ; MAP\n";
    print $outFH "FFFB; ; MAP\n";
    print $outFH "1D173; ; MAP\n";
    print $outFH "1D174; ; MAP\n";
    print $outFH "1D175; ; MAP\n";
    print $outFH "1D176; ; MAP\n";
    print $outFH "1D177; ; MAP\n";
    print $outFH "1D178; ; MAP\n";
    print $outFH "1D179; ; MAP\n";
    print $outFH "1D17A; ; MAP\n";
    print $outFH "E0001; ; MAP\n";
    print $outFH "E0020; ; MAP\n";
    print $outFH "E0021; ; MAP\n";
    print $outFH "E0022; ; MAP\n";
    print $outFH "E0023; ; MAP\n";
    print $outFH "E0024; ; MAP\n";
    print $outFH "E0025; ; MAP\n";
    print $outFH "E0026; ; MAP\n";
    print $outFH "E0027; ; MAP\n";
    print $outFH "E0028; ; MAP\n";
    print $outFH "E0029; ; MAP\n";
    print $outFH "E002A; ; MAP\n";
    print $outFH "E002B; ; MAP\n";
    print $outFH "E002C; ; MAP\n";
    print $outFH "E002D; ; MAP\n";
    print $outFH "E002E; ; MAP\n";
    print $outFH "E002F; ; MAP\n";
    print $outFH "E0030; ; MAP\n";
    print $outFH "E0031; ; MAP\n";
    print $outFH "E0032; ; MAP\n";
    print $outFH "E0033; ; MAP\n";
    print $outFH "E0034; ; MAP\n";
    print $outFH "E0035; ; MAP\n";
    print $outFH "E0036; ; MAP\n";
    print $outFH "E0037; ; MAP\n";
    print $outFH "E0038; ; MAP\n";
    print $outFH "E0039; ; MAP\n";
    print $outFH "E003A; ; MAP\n";
    print $outFH "E003B; ; MAP\n";
    print $outFH "E003C; ; MAP\n";
    print $outFH "E003D; ; MAP\n";
    print $outFH "E003E; ; MAP\n";
    print $outFH "E003F; ; MAP\n";
    print $outFH "E0040; ; MAP\n";
    print $outFH "E0041; ; MAP\n";
    print $outFH "E0042; ; MAP\n";
    print $outFH "E0043; ; MAP\n";
    print $outFH "E0044; ; MAP\n";
    print $outFH "E0045; ; MAP\n";
    print $outFH "E0046; ; MAP\n";
    print $outFH "E0047; ; MAP\n";
    print $outFH "E0048; ; MAP\n";
    print $outFH "E0049; ; MAP\n";
    print $outFH "E004A; ; MAP\n";
    print $outFH "E004B; ; MAP\n";
    print $outFH "E004C; ; MAP\n";
    print $outFH "E004D; ; MAP\n";
    print $outFH "E004E; ; MAP\n";
    print $outFH "E004F; ; MAP\n";
    print $outFH "E0050; ; MAP\n";
    print $outFH "E0051; ; MAP\n";
    print $outFH "E0052; ; MAP\n";
    print $outFH "E0053; ; MAP\n";
    print $outFH "E0054; ; MAP\n";
    print $outFH "E0055; ; MAP\n";
    print $outFH "E0056; ; MAP\n";
    print $outFH "E0057; ; MAP\n";
    print $outFH "E0058; ; MAP\n";
    print $outFH "E0059; ; MAP\n";
    print $outFH "E005A; ; MAP\n";
    print $outFH "E005B; ; MAP\n";
    print $outFH "E005C; ; MAP\n";
    print $outFH "E005D; ; MAP\n";
    print $outFH "E005E; ; MAP\n";
    print $outFH "E005F; ; MAP\n";
    print $outFH "E0060; ; MAP\n";
    print $outFH "E0061; ; MAP\n";
    print $outFH "E0062; ; MAP\n";
    print $outFH "E0063; ; MAP\n";
    print $outFH "E0064; ; MAP\n";
    print $outFH "E0065; ; MAP\n";
    print $outFH "E0066; ; MAP\n";
    print $outFH "E0067; ; MAP\n";
    print $outFH "E0068; ; MAP\n";
    print $outFH "E0069; ; MAP\n";
    print $outFH "E006A; ; MAP\n";
    print $outFH "E006B; ; MAP\n";
    print $outFH "E006C; ; MAP\n";
    print $outFH "E006D; ; MAP\n";
    print $outFH "E006E; ; MAP\n";
    print $outFH "E006F; ; MAP\n";
    print $outFH "E0070; ; MAP\n";
    print $outFH "E0071; ; MAP\n";
    print $outFH "E0072; ; MAP\n";
    print $outFH "E0073; ; MAP\n";
    print $outFH "E0074; ; MAP\n";
    print $outFH "E0075; ; MAP\n";
    print $outFH "E0076; ; MAP\n";
    print $outFH "E0077; ; MAP\n";
    print $outFH "E0078; ; MAP\n";
    print $outFH "E0079; ; MAP\n";
    print $outFH "E007A; ; MAP\n";
    print $outFH "E007B; ; MAP\n";
    print $outFH "E007C; ; MAP\n";
    print $outFH "E007D; ; MAP\n";
    print $outFH "E007E; ; MAP\n";
    print $outFH "E007F; ; MAP\n";

    #   ZERO WIDTH SPACE (U+200B) is mapped to nothing.  All other code
    #   points with Separator (space, line, or paragraph) property (e.g., Zs,
    #   Zl, or Zp) are mapped to SPACE (U+0020).  The following is a complete
    #   list of these code points: U+0020, 00A0, 1680, 2000-200A, 2028-2029,
    #   202F, 205F, 3000.

    print $outFH "200B; ; MAP\n";
    print $outFH "00A0; 0020; MAP\n";
    print $outFH "1680; 0020; MAP\n";
    print $outFH "2000; 0020; MAP\n";
    print $outFH "2001; 0020; MAP\n";
    print $outFH "2002; 0020; MAP\n";
    print $outFH "2003; 0020; MAP\n";
    print $outFH "2004; 0020; MAP\n";
    print $outFH "2005; 0020; MAP\n";
    print $outFH "2006; 0020; MAP\n";
    print $outFH "2007; 0020; MAP\n";
    print $outFH "2008; 0020; MAP\n";
    print $outFH "2009; 0020; MAP\n";
    print $outFH "200A; 0020; MAP\n";
    print $outFH "2028; 0020; MAP\n";
    print $outFH "2029; 0020; MAP\n";
    print $outFH "202F; 0020; MAP\n";
    print $outFH "205F; 0020; MAP\n";
    print $outFH "3000; 0020; MAP\n";

    print $outFH "\n# Total code points 238\n";
    close($outFH);
}
#-----------------------------------------------------------------------
sub usage {
    print << "END";
Usage:
filterRFC3454.pl
Options:
        --sourcedir=<directory>
        --destdir=<directory>
        --src-filename=<name of RFC file>
        --dest-filename=<name of destination file>
        --A1             Generate data for table A.1
        --B1             Generate data for table B.1
        --B2             Generate data for table B.2
        --B3             Generate data for table B.3
        --C11            Generate data for table C.1.1
        --C12            Generate data for table C.1.2
        --C21            Generate data for table C.2.1
        --C22            Generate data for table C.2.2
        --C3             Generate data for table C.3
        --C4             Generate data for table C.4
        --C5             Generate data for table C.5
        --C6             Generate data for table C.6
        --C7             Generate data for table C.7
        --C8             Generate data for table C.8
        --C9             Generate data for table C.9
        --iscsi          Generate data for iSCSI extra prohibited table
        --xmpp-node      Generate data for XMPP extra prohibited table
        --sasl           Generate data for SASL map table
        --ldap           Generate data for LDAP map table
        --normalize      Embed the normalization directive in the output file
        --check-bidi     Embed the check bidi directove in the output file

Note, --B2 and --B3 are mutually exclusive.

e.g.: filterRFC3454.pl --sourcedir=. --destdir=./output --src-filename=rfc3454.txt  --dest-filename=NamePrepProfile.txt --A1 --B1 --B2 --C12 --C22 --C3 --C4 --C5 --C6 --C7 --C8 --C9 --normalize --check-bidi

filterRFC3454.pl filters the RFC file and creates String prep table files.
The RFC text can be downloaded from ftp://ftp.rfc-editor.org/in-notes/rfc3454.txt

END
  exit(0);
}


