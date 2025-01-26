#!/usr/bin/perl -w

# regression testing for github issue #377
# (lines longer than the fixed buffer were split into multiple lines)

use Test::Command tests => 66;
use File::Temp;

{
# the issue was noticed with a very long target name (too long for DNS)
my $tmpfile = File::Temp->new();
print $tmpfile "a"x300 .".invalid\n";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: target name too long\n");
}

{
# the issue was noticed with a very long target name (too long for DNS)
# (no newline)
my $tmpfile = File::Temp->new();
print $tmpfile "a"x300 .".invalid";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: target name too long\n");
}

{
# a too long word can be found in two consecutive parts of the line
my $tmpfile = File::Temp->new();
print $tmpfile " "x200 ."a"x300 .".invalid\n";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: target name too long\n");
}

{
# a too long word can be found in two consecutive parts of the line
# (no newline)
my $tmpfile = File::Temp->new();
print $tmpfile " "x200 ."a"x300 .".invalid";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: target name too long\n");
}

{
# a too long word starting with the second part of the line (256B buffer)
my $tmpfile = File::Temp->new();
print $tmpfile " "x255 ."a"x300 .".invalid\n";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: target name too long\n");
}

{
# a too long word starting with the second part of the line (256B buffer)
# (no newline)
my $tmpfile = File::Temp->new();
print $tmpfile " "x255 ."a"x300 .".invalid";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: target name too long\n");
}

{
# first part of line read into buffer may be blank
my $tmpfile = File::Temp->new();
print $tmpfile " "x400 ."a"x300 .".invalid\n";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: target name too long\n");
}

{
# first part of line read into buffer may be blank
# (no newline)
my $tmpfile = File::Temp->new();
print $tmpfile " "x400 ."a"x300 .".invalid";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: target name too long\n");
}

{
# lines longer than the line buffer shall not be split - 132B buffer
my $tmpfile = File::Temp->new();
print $tmpfile " "x100 ."127.0.0.1 "." "x(131-9)."host.name.invalid\n";
print $tmpfile " "x122 ."127.0.0.2 "." "x(131-9)."host.name.invalid\n";
print $tmpfile " "x127 ."127.0.0.3 "." "x(131-9)."host.name.invalid\n";
print $tmpfile " "x131 ."127.0.0.4 "." "x(131-9)."host.name.invalid\n";
print $tmpfile " "x150 ."127.0.0.5 "." "x(131-9)."host.name.invalid\n";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive
127.0.0.2 is alive
127.0.0.3 is alive
127.0.0.4 is alive
127.0.0.5 is alive\n");
$cmd->stderr_is_eq("");
}

{
# lines longer than the line buffer shall not be split - 132B buffer
# (last line without newline)
my $tmpfile = File::Temp->new();
print $tmpfile " "x100 ."127.0.0.1 "." "x(131-9)."host.name.invalid\n";
print $tmpfile " "x122 ."127.0.0.2 "." "x(131-9)."host.name.invalid\n";
print $tmpfile " "x127 ."127.0.0.3 "." "x(131-9)."host.name.invalid\n";
print $tmpfile " "x131 ."127.0.0.4 "." "x(131-9)."host.name.invalid\n";
print $tmpfile " "x150 ."127.0.0.5 "." "x(131-9)."host.name.invalid";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive
127.0.0.2 is alive
127.0.0.3 is alive
127.0.0.4 is alive
127.0.0.5 is alive\n");
$cmd->stderr_is_eq("");
}

{
# lines longer than the line buffer shall not be split - 256B buffer
my $tmpfile = File::Temp->new();
print $tmpfile " "x240 ."127.0.0.1 "." "x(255-9)."host.name.invalid\n";
print $tmpfile " "x246 ."127.0.0.2 "." "x(255-9)."host.name.invalid\n";
print $tmpfile " "x251 ."127.0.0.3 "." "x(255-9)."host.name.invalid\n";
print $tmpfile " "x255 ."127.0.0.4 "." "x(255-9)."host.name.invalid\n";
print $tmpfile " "x275 ."127.0.0.5 "." "x(255-9)."host.name.invalid\n";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive
127.0.0.2 is alive
127.0.0.3 is alive
127.0.0.4 is alive
127.0.0.5 is alive\n");
$cmd->stderr_is_eq("");
}

{
# lines longer than the line buffer shall not be split - 256B buffer
# (last line without newline)
my $tmpfile = File::Temp->new();
print $tmpfile " "x240 ."127.0.0.1 "." "x(255-9)."host.name.invalid\n";
print $tmpfile " "x246 ."127.0.0.2 "." "x(255-9)."host.name.invalid\n";
print $tmpfile " "x251 ."127.0.0.3 "." "x(255-9)."host.name.invalid\n";
print $tmpfile " "x255 ."127.0.0.4 "." "x(255-9)."host.name.invalid\n";
print $tmpfile " "x275 ."127.0.0.5 "." "x(255-9)."host.name.invalid";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive
127.0.0.2 is alive
127.0.0.3 is alive
127.0.0.4 is alive
127.0.0.5 is alive\n");
$cmd->stderr_is_eq("");
}

{
# line without newline shorter than 131 bytes
my $tmpfile = File::Temp->new();
print $tmpfile " "x(131-10-9) ."127.0.0.1";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

{
# line without newline with 131 bytes
my $tmpfile = File::Temp->new();
print $tmpfile " "x(131-9) ."127.0.0.1";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

{
# line without newline with length between 131 and 255 bytes
my $tmpfile = File::Temp->new();
print $tmpfile " "x(255-10-9) ."127.0.0.1";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

{
# line without newline with 255 bytes
my $tmpfile = File::Temp->new();
print $tmpfile " "x(255-9) ."127.0.0.1";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

{
# line without newline with word split between two 256 byte buffers
my $tmpfile = File::Temp->new();
print $tmpfile " "x(255-5) ."127.0.0.1";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

{
# line without newline with between 255 and 510 bytes
my $tmpfile = File::Temp->new();
print $tmpfile " "x(255*2-10-9) ."127.0.0.1";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

{
# line without newline with 510 bytes
my $tmpfile = File::Temp->new();
print $tmpfile " "x(255*2-9) ."127.0.0.1";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

{
# invalid target name with a comment sign inside
my $tmpfile = File::Temp->new();
for (my $i = 1; $i < 4; $i++) {
    print $tmpfile " "x(255-11+$i)."127.0.0.$i#host.name.invalid\n";
}
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(2);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{127\.0\.0\.1\#host\.name\.invalid: .*
127\.0\.0\.2\#host\.name\.invalid: .*
127\.0\.0\.3\#host\.name\.invalid: .*\n});
}

{
# blank lines are ignored
my $tmpfile = File::Temp->new();
print $tmpfile " "x100 ."\n";
print $tmpfile " "x131 ."\n";
print $tmpfile " "x150 ."\n";
print $tmpfile " "x255 ."\n";
print $tmpfile " "x275 ."\n";
print $tmpfile " "x500 ."\n";
print $tmpfile " "x510 ."\n";
print $tmpfile " "x550 ."\n";
print $tmpfile " "x575;
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("");
}

{
# many comment lines, most needing more than one 256B buffer
my $tmpfile = File::Temp->new();
for (my $i = 1; $i < 4; $i++) {
    print $tmpfile " "x(255-11+$i)."# this is a comment\n";
}
for (my $i = 1; $i < 8; $i++) {
    print $tmpfile " "x(255-8+$i)."# x y z\n";
}
print $tmpfile "#"." "x300 ."localhost\n";
print $tmpfile " "x300 ."#"." "x300 ."127.0.0.1\n";
close($tmpfile);

my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("");
}
