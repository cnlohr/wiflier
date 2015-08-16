//Copyright (C) 2015 <>< Charles Lohr, see LICENSE file for more info.
//
//This particular file may be licensed under the MIT/x11, New BSD or ColorChord Licenses.

function AVRStatusCallback( is_ok, comment, pushop )
{
	if( is_ok )
	{
		$("#innerflashtext").html( comment );
		if( pushop.place == pushop.padlen )
		{
			var sendstr = "CAF" + flash_scratchpad_at + "\t" + pushop.padlen + "\t" + faultylabs.MD5( pushop.paddata ).toLowerCase() + "\n";
			var fun = function( fsrd, flashresponse ) { $("#innerflashtext").html( (flashresponse[0] == '!')?"Flashing failed.":"Flash success." ) };
			QueueOperation( sendstr, fun); 
			return false;
		}
	}
	else
	{
		$("#innerflashtext").html( "Failed: " + comment );
	}
	return true;
}

function DragDropAVRFile(file)
{
	if( file.length != 1 )
	{
		$("#innerflashtext").html( "Need exactly one file." );
		return;
	}
	if( file[0].size > 8192 )
	{
		$("#innerflashtext").html( "File too big to fit on AVR." );
		return;
	}

	$("#innerflashtext").html( "Opening " + file[0].name );

	var reader = new FileReader();

	reader.onload = function(e) {
		PushImageTo( e.target.result, flash_scratchpad_at, AVRStatusCallback );
	}

	
	reader.readAsArrayBuffer( file[0] );
}


