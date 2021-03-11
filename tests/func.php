<?php
/*
class CCC {
	public function test() {
		echo("in CCC::test");
	}
}
 */

sh_start();

function ttt() {
//function ttt($ddd) {
	echo("in ttt\r\n");
	//((string)($ddd))();


	//ttt_inner();
}


function ttt_inner() {
	echo("ttt_inner\r\n");
	ttt_inner_inner();
}
/*
function ttt_inner_inner() {
	echo("ttt_inner_inner\r\n");
}
*/

function sh_before_ttt() {
	echo("in ttt_hook before\r\n");
}

//function sh1_after_ttt() {
//	echo("in ttt_hook after\r\n");
//}

/*
ttt(function() {
	echo("ttt_param\r\n");
});
 */
//(new CCC())->test();

