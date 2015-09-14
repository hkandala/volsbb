<?php
    /*
     * Class which manages all features provided to user
     */

    require_once 'VOLSBB.class.inc';

    class User {
        public $accounts;
        public $defaultAccount;
        public $noOfAccounts;
        
        //Error Codes
        const ACCOUNT_ALREADY_EXISTS = 301;
        const ACCOUNT_NOT_FOUND = 302;
        const UNABLE_TO_WRITE = 303;
        
        public function __construct() {
            $this->accounts = [];
            $this->defaultAccount = null;
            $this->noOfAccounts = 0;
            file_put_contents(dirname(__FILE__) . '/data/user.txt', '', FILE_APPEND);
        }
        
        private function readData() {
            $data = file_get_contents(dirname(__FILE__) . '/data/user.txt');
            $accounts = explode(' $ ', $data);
            if($accounts[0] != '') {
                $n = 0;
                foreach($accounts as $account) {
                    $temp = explode(' | ', $account);
                    $accounts[$n] = [
                        'userName' => $temp[0],
                        'password' => $temp[1],
                        'renewDate' => $temp[2],
                        'default' => $temp[3]
                    ];
                    $n++;
                }
                return $accounts;
            } else {
                return [];
            }
        }
        
        private function writeData($accounts) {
            if(is_array($accounts) && count($accounts) >= 0) {
                $n = 0;
                $temp = [];
                foreach($accounts as $account) {
                    $temp[$n] = implode(' | ', $account);
                    $n++;
                }
                $string = implode(' $ ', $temp);
                file_put_contents(dirname(__FILE__) . '/data/user.txt', $string);
                return true;
            } else {
                return User::UNABLE_TO_WRITE;
            }
        }
        
        public function addAccount($userName, $password, $default = 0, $renewDate = 0) {
            $accountNew = [
                'userName' => $userName,
                'password' => $password,
                'renewDate' => $renewDate,
                'default' => $default
            ];
            $accounts = $this->readData();
            $n = 0;
            foreach($accounts as $account) {
                if($account['userName'] === $userName) {
                    return User::ACCOUNT_ALREADY_EXISTS;
                } else {
                    if($default == 1) {
                        $accounts[$n]['default'] = 0;
                    }
                }
                $n++;
            }
            $accounts[] = $accountNew;
            return $this->writeData($accounts);
        }
        
        public function removeAccount($userName) {
            $accounts = $this->readData();
            $n = 0;
            foreach($accounts as $account) {
                if($account['userName'] === $userName) {
                    unset($accounts[$n]);
                    return $this->writeData($accounts);
                }
                $n++;
            }
            return User::ACCOUNT_NOT_FOUND;
        }
        
        public function updateAccount($userName, $password, $default = 0) {
            if(($feedback = $this->removeAccount($userName)) === true) {
                return $this->addAccount($userName, $password, $default);
            } else {
                return $feedback;
            }
        }
        
        public function loadAllAccounts() {
            $accounts = $this->readData();
            foreach($accounts as $account) {
                if($account['renewDate'] > 0 && $account['renewDate'] <= 31){
                    $this->accounts[] = new VOLSBB($account['userName'], $account['password'], $account['renewDate']);
                } else {
                    $this->accounts[] = new VOLSBB($account['userName'], $account['password']);
                }
                if($account['default'] == 1) {
                    if($account['renewDate'] > 0 && $account['renewDate'] <= 31){
                        $this->defaultAccount = new VOLSBB($account['userName'], $account['password'], $account['renewDate']);
                    } else {
                        $this->defaultAccount = new VOLSBB($account['userName'], $account['password']);
                    }
                }
            }
            $this->noOfAccounts = count($this->accounts);
        }
    };