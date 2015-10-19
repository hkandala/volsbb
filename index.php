<?php
    ini_set('max_execution_time', 60);

	require_once 'include/php/VOLSBB.class.inc';

    $volsbb = new VOLSBB('harish095', 'hsirah9908897909');

    $volsbb->refreshSession();

    if(($feedback = $volsbb->getUsageData($result)) === true) {
        echo '<pre>';
        print_r($result);
        echo '</pre>';
    } else {
        echo $feedback;
    }