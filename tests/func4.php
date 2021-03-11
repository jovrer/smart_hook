<?php

$funcs = [
	'before'=>function() {
		echo("before\r\n");
	},
	'after'=>function() {
		echo("after\r\n");
	}
];

$func_name = "ttt";
sh_start($func_name, $funcs);
//sh_start($func_name);


function ttt() {
	echo("in ttt\r\n");
}

ttt();
