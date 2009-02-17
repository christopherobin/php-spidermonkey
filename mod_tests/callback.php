<?php

$a = new JSRuntime();
$b = $a->createContext();
$b->registerFunction('printf', 'printf');

$assScr = <<<SCR
lf = String.fromCharCode(10);
for (var idx in env)
{
    printf('[%s] : %s', idx, env[idx] + lf);
}
SCR;
$b->assign('env', $_ENV);
$b->evaluateScript($assScr);
