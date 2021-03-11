<?php
$dd = [
	'before'=>function() {
		echo("before");
	},
	'after'=>function() {
		echo("after");
	}
];

$dd['before']();
