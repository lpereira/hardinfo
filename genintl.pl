#!/usr/bin/perl
#
# Script para gerar arquivo de internacionalização
# Versão 1.0
#
# Copyright (c) 2002-2003 Leandro Pereira <leandro@linuxmag.com.br>
# Copyright (c) 2003      RadSys Software Ltda.
# Todos os direitos reservados.
#
# É permitida a distribuição e modificação deste, desde que os créditos
# estejam presentes e que exista uma notificação sobre as modificações
# feitas.
# Não há restrição de uso.
#

print "Generating `default.lang' catalog...\n";

@list=`find *.c`;

$maxsize=0;
foreach $i (0..$#list){
	$maxsize=length($list[$i]) if(length($list[$i]) > $maxsize);
}

open(B, ">default.lang");

print B "[Translation]\n";
print B "translated-by=Unknown\n\n";

$messages=0;

foreach $k (0..$#list){
	$line=0;
	$thismsg=0;

	$file=$list[$k];
	chomp($file);
	print B "[$file]\n";
	print STDERR "Searching in `$file':";
	print STDERR " " x ($maxsize - length($file) + 2);

	open(A, $file);
	while(<A>){
		$line++;
		if(/_\(/){
			chomp;
			$_=~s/\t//g;
			$_=~s/_ \(/_\(/g;
	
			for($i=0; $i<=length($_); $i++){
				if(substr($_, $i, 1) eq "_" &&
				    substr($_, $i+1, 1) eq "\("){			
					for($j=$i+3; substr($_, $j, 1) ne "\""; $j++){
						print B substr($_, $j, 1);
					}
					print B "=";
					for($j=$i+3; substr($_, $j, 1) ne "\""; $j++){
						print B substr($_, $j, 1);
					}
					print B "\n";

					$messages++;
					$thismsg++;
				}
			}
		}
	}
	close(A);
	print B "\n";
	if($thismsg){
		printf "%02d messages", $thismsg;
	}else{
		print "Nothing";
	}
	print " found.\n";
}

close(B);

print "$messages messages saved in `default.lang'\n";
