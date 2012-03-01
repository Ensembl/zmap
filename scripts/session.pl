#!/usr/bin/perl
#
# simple script to process an otterlace session directory ZMap config and make a standalone system
# cached GFF used forfile servers
# ACEDB session exported into GFF
# all data held in one flat directory and paths adjusted
#
# mh17@sanger.ac.uk 29 Feb 2012



use strict;
use warnings;

use Cwd;

my $otter_dir;
my $zmap_dir;

my $dataset = "human";


sub copy_file
{
	my $file = shift;
	my $dest = $zmap_dir;
	my $ofile = shift;
	$dest .= '/' . $ofile if ($ofile);

	my $command =  "cp $file $dest";
	system $command;
}

sub main
{
	my $thing = "session";

	$otter_dir = shift;
	$zmap_dir = shift;

	die("usage session.pl <otterlace_directory> <zmap_directory>") if (!$zmap_dir);

	$zmap_dir = getcwd() . '/' . $zmap_dir if (!($zmap_dir =~ '^/'));

	@_ = split ('/',$0);
	my $script = pop;
	$thing = "ZMap config" if($script ne "session.pl");

	printf ("Extracting $thing from $otter_dir to $zmap_dir\n");

	die("cannot acess $zmap_dir") if (!(-w $zmap_dir) && ! mkdir($zmap_dir,0744));
	die("cannot acess $otter_dir") if (!chdir($otter_dir));

	if($script eq "session.pl")
	{
		chdir("$otter_dir/gff_cache");
		opendir DIR, '.';
		my @gff_files = (readdir DIR);
		foreach my $gff (@gff_files)
		{
			next if ($gff =~ '^\.');

			printf("extracting $gff\n");

			my @bits = split('_', $gff);
			pop @bits;
			my $file = join('_', @bits) . '.gff';
			copy_file($gff, $file);
		}
		closedir DIR;
	}

	print "extracting ZMap config\n";

	chdir("$otter_dir/ZMap");

	copy_file('styles.ini');

	# need to process blixemrc or replace it


	# filter ZMAP and replace pipe servers and acedb with file servers
	open (ZMAP, "ZMap") or die("cannot read ZMap config file");
	open(OFILE,"> " . $zmap_dir . "/ZMap") or die("cannot open ZMap for output\n");
	open(CFILE,">" . $zmap_dir . "/commands.txt") or die("cannot open commands.txt");
	print CFILE "##!/bin/bash\n# commands to cache BAM data: run after otterlace_env first from relevant release\n";
	print CFILE ". /software/anacode/otter/otter_rel63/bin/otterlace_env.sh\n\n";

	while (<ZMAP>)
	{
		chomp;

		s/false/true/ if(/show-mainwindow/);
		s/true/false/ if(/thread-fail-silent/);
		my $oline = $_;

		# for all filters:
		# patch styles file to runtime directory
		# change pipe://filter_get into file://
		# change bam_get and bam_get_align into file://
		# change bam_get_coverage into file://
		# NOTE need to cache BAM data - write sample commands to another file and do by hand

		$oline = "stylesfile=$zmap_dir/styles.ini" if ( /stylesfile/ ) ;


		next if (/data-dir/);		# loose any old settings
		if(/\[ZMap\]/)
		{
			$oline .= "\ndata-dir=$zmap_dir";		# ensure this is set
		}

		$dataset = $1 if (/dataset\s*=\s*(\w+)[\W\$]/);

=begin comment
[ensembl_bodymap_blood]
delayed=true
featuresets=ensembl_bodymap_blood
group=always
stylesfile=/var/tmp/lace_63.mh17.21836.4/ZMap/styles.ini
url=pipe:///filter_get?analysis=blood_rnaseq&client=otterlace&cookie_jar=%2Fnfs%2Fusers%2Fnfs_m%2Fmh17%2F.otter%2Fns_cookie_jar&cs=chromosome&csver=Otter&dataset=human&end=1127268&gff_seqname=chr1-14&gff_source=ensembl_bodymap_blood&metakey=ens_bodymap_db&name=1&server_script=get_gff_genes&session_dir=%2Fvar%2Ftmp%2Flace_63.mh17.21836.4&start=812485&transcript_analyses=blood_rnaseq&type=chr1-14&url_root=http%3A%2F%2Fdev.sanger.ac.uk%3A80%2Fcgi-bin%2Fotter%2F63

[Tier1_H1-hESC_cytosol_longPolyA_rep2]
delayed=true
featuresets=Tier1_H1-hESC_cytosol_longPolyA_rep2
group=always
stylesfile=/var/tmp/lace_63.mh17.21836.4/ZMap/styles.ini
url=pipe:///bam_get_align?--chr=chr1-14&--chr_prefix=chr&--csver=GRCh37&--end=1127268&--file=http%3A%2F%2Fhgdownload-test.cse.ucsc.edu%2FgoldenPath%2Fhg19%2FencodeDCC%2FwgEncodeCshlLongRnaSeq%2FreleaseLatest%2FwgEncodeCshlLongRnaSeqH1hescCytosolPapAlnRep2.bam&--gff_feature_source=Tier1_H1-hESC_cytosol_longPolyA_rep2&--start=812485&-gff_version=2

[Tier3_SK-N-SH_RA_cell_longPolyA_rep2_coverage_minus]
delayed=true
featuresets=Tier3_SK-N-SH_RA_cell_longPolyA_rep2_coverage_minus
group=always
stylesfile=/var/tmp/lace_63.mh17.21836.4/ZMap/styles.ini
url=pipe:///bigwig_get?-chr=chr1-14&-chr_prefix=chr&-csver=GRCh37&-end=1127268&-file=http%3A%2F%2Fhgdownload-test.cse.ucsc.edu%2FgoldenPath%2Fhg19%2FencodeDCC%2FwgEncodeCshlLongRnaSeq%2FreleaseLatest%2FwgEncodeCshlLongRnaSeqSknshraCellPapMinusRawSigRep2.bigWig&-gff_feature_source=Tier3_SK-N-SH_RA_cell_longPolyA_rep2_coverage_minus&-gff_version=2&-start=812485&-strand=-1
=cut

		if(/url=/)
		{
			if(/acedb/)
			{
				$oline = "url=file:///$zmap_dir/session.gff";
			}
			elsif(/filter_get/)
			{
				# get the gff_source argument
				if(/gff_source=([\w-]+)[&\$]/)
				{
					$oline = "url=file:///$zmap_dir/$1.gff";
				}
				else
				{
					die("no pipe gff_source in $_");
				}
			}
			elsif(/bam_get/ || /bigwig_get/)
			{
				if(/gff_feature_source\s*=\s*([\w-]+)\W/)
				{
					my $set = $1;
					$oline = "url=file:///$zmap_dir/$set.gff";

					# output command to cache the data
					s|url=pipe:/+||;
					s/\?/ /;
					s/\&/ /g;
					s:%2F:/:g;
					print CFILE $_ , " --dataset=$dataset > $zmap_dir/$set.gff\n";
				}
				else
				{
					die("no pipe gff_feature_source in $_");
				}
			}
			elsif(/pipe/)
			{
				die("unknown pipe script in $_");
			}
			else
			{
				die("unexpected url type in $_");
			}
		}

		print OFILE $oline, "\n";
	}
}



main(@ARGV);

