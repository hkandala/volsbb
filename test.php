<?php
    ini_set('max_execution_time', 60);

	require_once 'include/php/User.class.inc';

    $user = new User();
    $user->addAccount('harish095', 'hsirah9908897909', 1, 1);

    $user->loadAllAccounts();

    if(($feedback = $user->accounts[0]->getUsageData($result)) === true) {
        echo '<pre>';
        print_r($result);
        echo '</pre>';
    } else {
        echo $feedback;
    }