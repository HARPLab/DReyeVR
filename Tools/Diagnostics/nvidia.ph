# copyright, 2003-2009 Hewlett-Packard Development Company, LP

# NOTE - by default, this module only collects data once a minute, which you can change
# with the i=x parameter (eg --import nvidia,i=x).  Regardless of the collection interval,
# date will be reported every interval in brief/verbose formats to provide a consisent 
# set of output each monitoring cycle.

#    n v d i a    m o n i t o r i n g

use strict;

# Allow reference to collectl variables, but be CAREFUL as these should be treated as readonly
our ($Version, $miniFiller, $rate, $SEP, $datetime, $miniInstances, $interval, $showColFlag, $playback);

my ($nvidiaInterval, $nvidiaImportCount, $nvidiaSampleCounter, $nvidiaAllFlag);
my ($nvidiaOpts,    $nvidiaNumGpus,$nvidiaProdName,   $nvidiaDriverVer);
my (@nvidiaTemp,    @nvidiaFan,    @nvidiaGpuUtil,    @nvidiaMemUtil);
my ($nvidiaTempTot, $nvidiaFanTot, $nvidiaGpuUtilTot, $nvidiaMemUtilTot);
my ($nvidiaTempTOT, $nvidiaFanTOT, $nvidiaGpuUtilTOT, $nvidiaMemUtilTOT);

my $nvidiaGpuFirst;
my $nvidiaGpuNum=-1;
my $nvidiaSection=0;
my $nvidiaLastValFlag;
my $nvidiaCommand='/usr/bin/nvidia-smi';
my $nvidiaFullCmd="$nvidiaCommand -q -a";

sub nvidiaInit
{
  my $impOptsref=shift;
  my $impKeyref= shift;

  # If we ever run with a ':' in the interval, we need to be sure we're
  # only looking at the main one.
  my $nvidiaInterval1=(split(/:/, $interval))[0];

  # For now, only options are a, 'i=, d, s and sd
  $nvidiaOpts='';
  $nvidiaInterval=60;
  if (defined($$impOptsref))
  {
    foreach my $option (split(/,/,$$impOptsref))
    {
      my ($name, $value)=split(/=/, $option);
      error("invalid misc option: '$name'")    if $name ne 'a' && $name ne 'i' && $name!~/^[ds]+$/;

      $nvidiaInterval=$value    if $name eq 'i';
      $nvidiaAllFlag=1          if $name eq 'a';
      $nvidiaOpts=$name         if $name=~/^[ds]+$/;
    }
  }

  # Special case: if --showcowheaders, we're called with -i0 and need to reset so the
  # division below doesn't bomb out.
  $nvidiaInterval1=1    if $showColFlag;
  $nvidiaImportCount=int($nvidiaInterval/$nvidiaInterval1);
  error("nvidia interval option not a multiple of '$nvidiaInterval1' seconds")
        if $nvidiaInterval1*$nvidiaImportCount != $nvidiaInterval;

  $nvidiaOpts='s'    if $nvidiaOpts eq '';
  $$impOptsref=$nvidiaOpts;
  $$impKeyref='nvid';

  # since totals only initialized in 'analyze' routine, do it here too in case no data
  # in raw file during playback.
  $nvidiaTempTot=$nvidiaFanTot=$nvidiaGpuUtilTot=$nvidiaMemUtilTot=0;

  #    R u n    C o m m a n d    I n    R e a l t i m e    M o d e

  # when running in collection mode only, we need to make sure we're nvidia enabled
  if ($playback eq '')
  {
    if ($playback eq '' && !-e $nvidiaCommand || $Version lt '3.5.0-3')
    {
      my $message="cannot find '$nvidiaCommand'"                     if !-e $nvidiaCommand;
      $message="this nvidia.ph requires at least collectl V3.5.1"    if $Version lt '3.5.1';
      pushmsg('W', "$message.  GPU monitoring disabled");
      $$impOptsref=$$impKeyref='';
      return(-1);
    }

    # Now run command once to get a get a few constants for header AND number of GPUs
    # NOTE: if it returns its names its probably a help message and the desired commands
    # are not suppoted, hence not a GPU we can monitor.
    my $output=`$nvidiaFullCmd`;
    if ($output=~/nvidia-smi/)
    {
      pushmsg('W', "nvidia GPU doesn' support '$nvidiaFullCmd'.  GPU monitoring disabled");
      $$impOptsref=$$impKeyref='';
      return(-1);
    }

    $nvidiaNumGpus=0;
    foreach my $line ((split(/\n/, $output)))
    {
      chomp $line;
      $nvidiaDriverVer=$1    if $line=~/Driver Version\s+:\s+(.*)/;

      if ($line=~/Product Name\s+:\s+(.*)/)
      {
        $nvidiaProdName=$1;
        $nvidiaNumGpus++;
      }
    }
    $nvidiaSampleCounter=-1;
  }

  return(1);
}

# Nothing to add to header
sub nvidiaUpdateHeader
{
  my $lineref=shift;
  $$lineref.="# GPU:        Type: nvidia $nvidiaProdName  Version: $nvidiaDriverVer  NumGPU: $nvidiaNumGpus\n";
}

sub nvidiaGetHeader
{
  my $headerref=shift;

  # Make sure playback file contains nvidia gpu data and if not, just set number
  # of gpus to 1 to make display logic work because maybe this is intended.
  if ($$headerref!~/GPU:(.*)Version/ || $1!~/nvidia/)
  {
    logmsg('W', "playback file does not contain nvidia data");
    $nvidiaNumGpus=1;
  }
  else
  {
    # Now parse header for real
    $$headerref=~/GPU:.*Type: (.*).*Version: (.*)\s+NumGPU: (\d+)/;
    $nvidiaDriverVer=$2;
    $nvidiaNumGpus=$3;
  }
}

sub nvidiaGetData
{
  # only read counters when OUR interval directs us to.
  return    if ($nvidiaSampleCounter++ % $nvidiaImportCount)!=0;

  getExec(0, $nvidiaFullCmd, 'nvid');
}

sub nvidiaInitInterval
{
  # all initialization totals MUST be done in Analyze() because this not called
  # each pass when writing to a raw file.
}

sub nvidiaAnalyze
{
  my $type=   shift;
  my $dataref=shift;

  # we need to know when last counter seen AND we can't use InitInterval()
  $nvidiaLastValFlag=0;

  # This is a mess!  Difference drivers generate different output
  if ($nvidiaDriverVer lt '270.41.19')    # until I know better
  {
    # GPU number always preceeds data
    $nvidiaGpuNum=$1      if $$dataref=~/^GPU (\d+)/;  

    my $i=$nvidiaGpuNum;
    $nvidiaTemp[$i]=$1       if $$dataref=~/^Temp.*?(\d+)/;
    $nvidiaFan[$i]=$1        if $$dataref=~/^Fan.*?(\d+)/;
    $nvidiaGpuUtil[$i]=$1    if $$dataref=~/^GPU.*: (\d+)/;

    # This indicates the end of a set of data.  If more data elements
    # are ever added, we'll need a different 'last' variable test
    if ($$dataref=~/^Memory.*?(\d+)/)
    {
      $nvidiaMemUtil[$i]=$1;
      $nvidiaLastValFlag=1;
    }
  }
  elsif ($nvidiaDriverVer ge '270.41.19')
  {
    # for this driver version this isn't a number but a string so just bump index
    # and make a note of it so we recognize the start of a new interval
    if ($$dataref=~/^GPU (.*)/ && $$dataref!~/UUID/)
    {
      $nvidiaGpuFirst=$1     if !defined($nvidiaGpuFirst);
      $nvidiaGpuNum=-1       if $1 eq $nvidiaGpuFirst;
      $nvidiaGpuNum++;
    }

    $nvidiaSection=10      if $$dataref=~/^PCI/;
    $nvidiaSection=20      if $$dataref=~/^Fan/;
    $nvidiaSection=30      if $$dataref=~/^Memory Usage/;
    $nvidiaSection=40      if $$dataref=~/^Utilization/;
    $nvidiaSection=50      if $$dataref=~/^Ecc Mode/;
    $nvidiaSection=60      if $$dataref=~/^ECC Errors/;
    $nvidiaSection=70      if $$dataref=~/^Temperature/;
    $nvidiaSection=80      if $$dataref=~/^Power Readings/;
    $nvidiaSection=90      if $$dataref=~/^Clocks/;

    # for now we only care about Fan, Utiliztion and Temp
    return    if  $nvidiaSection!=20 && $nvidiaSection!=40 && $nvidiaSection!=70;

    my $i=$nvidiaGpuNum;
    $nvidiaFan[$i]=$1        if $nvidiaSection==20 && $$dataref=~/^Fan.*?(\d+)/;
    $nvidiaGpuUtil[$i]=$1    if $nvidiaSection==40 && $$dataref=~/^Gpu.*: (\d+)/;
    $nvidiaMemUtil[$i]=$1    if $nvidiaSection==40 && $$dataref=~/^Memory.*?(\d+)/;

    if ($nvidiaSection==70 && $$dataref=~/^Gpu.*?(\d+)/)
    {
      $nvidiaTemp[$i]=$1;
      $nvidiaLastValFlag=1;
    }
  }

  # We can only do initialization and update totals when we see last data element (actually
  # we could have updated the totals as each element seen but this is a little easier.
  if ($nvidiaLastValFlag)
  {
    my $i=$nvidiaGpuNum;

    # These only done once/interval, not per GPU
    $nvidiaTempTot=$nvidiaFanTot=$nvidiaGpuUtilTot=$nvidiaMemUtilTot=0    if $i==0;

    $nvidiaTempTot+=   $nvidiaTemp[$i];
    $nvidiaFanTot+=    $nvidiaFan[$i];
    # $nvidiaGpuUtilTot+=$nvidiaGpuUtil[$i];
    # $nvidiaMemUtilTot+=$nvidiaMemUtil[$i];
  }
}

sub nvidiaPrintBrief
{
  my $type=shift;
  my $lineref=shift;

  # DReyeVR edits here, added explicit call to nvidia-smi querying gpu/memory utilization 
  $nvidiaGpuUtilTot=`nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits`;
  $nvidiaMemUtilTot=`nvidia-smi --query-gpu=utilization.memory --format=csv,noheader,nounits`;

  if ($type==1)       # header line 1
  {
    $$lineref.="<------nvidia------->";
  }
  elsif ($type==2)    # header line 2
  {
    $$lineref.=" Temp Fan% Gpu% Mem% ";
  }
  elsif ($type==3)    # data
  {
    my $num=$nvidiaNumGpus;
    $$lineref.=sprintf(" %4d %4d %4d %4d ",
	$nvidiaTempTot/$num,    $nvidiaFanTot/$num, $nvidiaGpuUtilTot/$num, $nvidiaMemUtilTot/$num);
  }
  elsif ($type==4)    # reset 'total' counters
  {
    $nvidiaTempTOT=$nvidiaFanTOT=$nvidiaGpuUtilTOT=$nvidiaMemUtilTOT=0;
  }
  elsif ($type==5)    # increment 'total' counters
  {
    $nvidiaTempTOT+=   $nvidiaTempTot;
    $nvidiaFanTOT+=    $nvidiaFanTot;
    $nvidiaGpuUtilTOT+=$nvidiaGpuUtilTot;
    $nvidiaMemUtilTOT+=$nvidiaMemUtilTot;
  }
  elsif ($type==6)    # print 'total' counters
  {
    my $num=$nvidiaNumGpus;
    printf(" %4d %4d %4d %4d ",
	int($nvidiaTempTOT/$num/$miniInstances),    int($nvidiaFanTOT/$num/$miniInstances), 
	int($nvidiaGpuUtilTOT/$num/$miniInstances), int($nvidiaMemUtilTOT/$num/$miniInstances));
  }
}

sub nvidiaPrintVerbose
{
  my $printHeader=shift;
  my $homeFlag=   shift;
  my $lineref=    shift;

  # DReyeVR edits here, added explicit call to nvidia-smi querying gpu/memory utilization 
  $nvidiaGpuUtilTot=`nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits`;
  $nvidiaMemUtilTot=`nvidia-smi --query-gpu=utilization.memory --format=csv,noheader,nounits`;

  if ($nvidiaOpts=~/s/)
  {
    my $line='';
    if ($printHeader)
    {
      $line.="\n"    if !$homeFlag;
      $line.="# GPU STATISTICS\n";
      $line.="#$miniFiller Temp Fan% GPU% Mem%\n";
    }

    my $num=$nvidiaNumGpus;
    $$lineref=$line;
    $$lineref.=sprintf("$datetime   %3d  %3d  %3d  %3d\n",
	int($nvidiaTempTot/$num),    int($nvidiaFanTot/$num), 
	int($nvidiaGpuUtilTot/$num), int($nvidiaMemUtilTot/$num));
  }

  if ($nvidiaOpts=~/d/)
  {
    my $line='';
    if ($printHeader)
    {
      $line.="\n"    if !$homeFlag;
      $line.="# GPU DETAIL\n";
      $line.="#$miniFiller Num Temp Fan% GPU% Mem%\n";
    }

    $$lineref=$line;
    for (my $i=0; $i<$nvidiaNumGpus; $i++)
    {
      $$lineref.=sprintf("$datetime  %3d  %3d  %3d  %3d  %3d\n",
		$i, $nvidiaTemp[$i], $nvidiaFan[$i], $nvidiaGpuUtil[$i], $nvidiaMemUtil[$i])
    }
  }
}

sub nvidiaPrintPlot
{
  my $type=   shift;
  my $ref1=   shift;

  # DReyeVR edits here, added explicit call to nvidia-smi querying gpu/memory utilization 
  $nvidiaGpuUtilTot=`nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits`;
  $nvidiaMemUtilTot=`nvidia-smi --query-gpu=utilization.memory --format=csv,noheader,nounits`;

  #    H e a d e r s

  # Summary
  if ($type==1 && $nvidiaOpts=~/s/)
  {
    $$ref1.="[NVIDIA]Temp${SEP}[NVIDIA]Fan${SEP}[NVIDIA]Gpu${SEP}[NVIDIA]Mem${SEP}"
  }

  # Detail
  if ($type==2 && $nvidiaOpts=~/d/)
  {
    for (my $i=0; $i<$nvidiaNumGpus; $i++)
    {
      $$ref1.="[NVIDIA:$i]Temp${SEP}[NVIDIA:$i]Fan${SEP}[NVIDIA:$i]Gpu${SEP}[NVIDIA:$i]Mem${SEP}"
    }
  }

  #    D a t a

  # Summary
  if ($type==3 && $nvidiaOpts=~/s/)
  {
    my $num=$nvidiaNumGpus;
    $$ref1.=sprintf("$SEP%d$SEP%d$SEP%d$SEP%d",
	$nvidiaTempTot/$num,    $nvidiaFanTot/$num, $nvidiaGpuUtilTot/$num, $nvidiaMemUtilTot/$num)
  }

  # Detail
  if ($type==4 && $nvidiaOpts=~/d/)
  {
    for (my $i=0; $i<$nvidiaNumGpus; $i++)
    {
      $$ref1.=sprintf("$SEP%d$SEP%d$SEP%d$SEP%d",
	  $nvidiaTemp[$i], $nvidiaFan[$i], $nvidiaGpuUtil[$i], $nvidiaMemUtil[$i]);
    }
  }
}

sub nvidiaPrintExport
{
  my $type=   shift;
  my $ref1=   shift;
  my $ref2=   shift;
  my $ref3=   shift;

  # DReyeVR edits here, added explicit call to nvidia-smi querying gpu/memory utilization 
  $nvidiaGpuUtilTot=`nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits`;
  $nvidiaMemUtilTot=`nvidia-smi --query-gpu=utilization.memory --format=csv,noheader,nounits`;

  # counters are only returned based on "i=" or default of 60 seconds
  return    if !$nvidiaAllFlag && (($nvidiaSampleCounter-1) % $nvidiaImportCount)!=0;

  if ($nvidiaOpts=~/s/)
  {
    if ($type eq 'l')
    {
     push @$ref1, "nvidia.temp";   push @$ref2, sprintf("%d", $nvidiaTempTot/$nvidiaNumGpus);
     push @$ref1, "nvidia.fan";    push @$ref2, sprintf("%d", $nvidiaFanTot/$nvidiaNumGpus);
     push @$ref1, "nvidia.gpu";    push @$ref2, sprintf("%d", $nvidiaGpuUtilTot/$nvidiaNumGpus);
     push @$ref1, "nvidia.mem";    push @$ref2, sprintf("%d", $nvidiaMemUtilTot/$nvidiaNumGpus);
    }
    elsif ($type eq 's')
    {
      my $num=$nvidiaNumGpus;
      $$ref1.=sprintf("(nvidiatot (temp %d)(fan %d)(gpu %d)(mem %d))\n", 
		$nvidiaTempTot/$num, $nvidiaFanTot/$num, $nvidiaGpuUtilTot/$num, $nvidiaMemUtilTot/$num);
    }
    elsif ($type eq 'g')
    {
     push @$ref2, 'pct', 'pct', 'pct', 'pct';
     push @$ref1, "nvidia.temp";   push @$ref3, sprintf("%d", $nvidiaTempTot/$nvidiaNumGpus);
     push @$ref1, "nvidia.fan";    push @$ref3, sprintf("%d", $nvidiaFanTot/$nvidiaNumGpus);
     push @$ref1, "nvidia.gpu";    push @$ref3, sprintf("%d", $nvidiaGpuUtilTot/$nvidiaNumGpus);
     push @$ref1, "nvidia.mem";    push @$ref3, sprintf("%d", $nvidiaMemUtilTot/$nvidiaNumGpus);
    }
  }

  if ($nvidiaOpts=~/d/)
  {
    if ($type=~/[gl]/)
    {
      for (my $i=0; $i<$nvidiaNumGpus; $i++)
      {
        if ($type eq 'l')
        {
          push @$ref1, "nvidia.temp$i";   push @$ref2, sprintf("%d", $nvidiaTemp[$i]);
          push @$ref1, "nvidia.fan$i";    push @$ref2, sprintf("%d", $nvidiaFan[$i]);
          push @$ref1, "nvidia.gpu$i";    push @$ref2, sprintf("%d", $nvidiaGpuUtil[$i]);
          push @$ref1, "nvidia.mem$i";    push @$ref2, sprintf("%d", $nvidiaMemUtil[$i]);
        }
        elsif ($type eq 'g')
        {
          push @$ref2, 'pct', 'pct', 'pct', 'pct';
          push @$ref1, "nvidia.temp$i";   push @$ref3, sprintf("%d", $nvidiaTemp[$i]);
          push @$ref1, "nvidia.fan$i";    push @$ref3, sprintf("%d", $nvidiaFan[$i]);
          push @$ref1, "nvidia.gpu$i";    push @$ref3, sprintf("%d", $nvidiaGpuUtil[$i]);
          push @$ref1, "nvidia.mem$i";    push @$ref3, sprintf("%d", $nvidiaMemUtil[$i]);
        } 
      }
    }
    elsif ($type eq 's')
    {
      $$ref2.="(nvidiainfo\n";
      $$ref2.="  (num";
      for (my $i=0; $i<$nvidiaNumGpus; $i++)
      { $$ref2.=" $i"; }
      $$ref2.=")\n";

      my ($temp, $fan, $gpu, $mem)=('','','','');
      for (my $i=0; $i<$nvidiaNumGpus; $i++)
      {
        $temp.=" $nvidiaTemp[$i]";
        $fan.= " $nvidiaFan[$i]";
        $gpu.= " $nvidiaGpuUtil[$i]";
        $mem.= " $nvidiaMemUtil[$i]";
      }

      $$ref2.="  (temp$temp)\n";
      $$ref2.="  (fan$fan)\n";
      $$ref2.="  (gpu$gpu)\n";
      $$ref2.="  (mem$mem))\n";
    }
  }
}

1;
