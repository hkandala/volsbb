<?php
    /*
     * Class which manages all VOLSBB functions
     */

    require_once 'curl.inc';

    class VOLSBB {
        public $username;
        public $password;
        public $status;
        public $planSize;
        public $renewDate;
        public $userDefinedRenew;

        //Status Codes
        const LOGGED_IN = 101;
        const MANAGER_LOGGED_IN = 102;
        const LOGGED_OUT = 103;
        const WIFI_OFF = 104; //Can be any network problems
        const WIFI_UNKNOWN = 105;
        const UNKNOWN_ERROR = 106;

        //Error Codes
        const UNEXPECTED_RESPONSE = 201;
        const CURL_ERROR = 202; //Can be VOLSBB not responding
        const INVALID_PAGE = 203;
        const INVALID_PASSWORD = 204;
        const INVALID_USERNAME = 205;
        const QUOTA_OVER = 206;
        const ACCOUNT_EXPIRED = 207;
        const UNKNOWN_LOGIN_ERROR = 208;

        public function __construct($username, $password, $renewDate = null) {
            $this->username = $username;
            $this->password = $password;
            if($renewDate != null) {
                $this->renewDate = $renewDate;
                $this->userDefinedRenew = true;
            } else {
                $this->userDefinedRenew = false;
            }
        }

        public function checkStatus() {
            $curl = curl_init();
            curl_setopt($curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows; U; Windows NT 5.1; rv:1.7.3) Gecko/20041001 Firefox/0.10.1");
            curl_setopt($curl, CURLOPT_URL, 'http://phc.prontonetworks.com/cgi-bin/authlogin');
            curl_setopt($curl, CURLOPT_FRESH_CONNECT, true);
            curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($curl, CURLOPT_CONNECTTIMEOUT, 20);
            curl_setopt($curl, CURLOPT_TIMEOUT, 20);
            $response = curl_exec($curl);
            $error = curl_errno($curl);

            if($error == 0) {
                if(strpos($response, 'You are already logged in')) {
                    $this->status = VOLSBB::LOGGED_IN;
                } else {
                    $response2 = curl('http://115.248.50.60/registration/main.do?content_key=%2FSelectedPlan.jsp');
                    if($response2['error'] == null) {
                        if(strpos($response2['html'], 'Subscribed Plan Details')) {
                            $this->status = VOLSBB::MANAGER_LOGGED_IN;
                        } else {
                            $this->status = VOLSBB::LOGGED_OUT;
                        }
                    } else if($response2['error'] == 5 || 6 || 7) {
                        $this->status = VOLSBB::WIFI_OFF;
                    } else {
                        $this->status = VOLSBB::UNKNOWN_ERROR;
                    }
                }
            } else if($error == 52) {
                $this->status = VOLSBB::WIFI_UNKNOWN;
            } else if($error == 5 || 6 || 7) {
                $this->status = VOLSBB::WIFI_OFF;
            } else {
                $this->status = VOLSBB::UNKNOWN_ERROR;
            }
        }

        public function checkLoginResponse($html) {
            if(strpos($html, 'account does not exist')) {
                return VOLSBB::INVALID_USERNAME;
            } else if(strpos($html, 'check your username and password') || strpos($html, 'password was not accepted')) {
                return VOLSBB::INVALID_PASSWORD;
            } else if(strpos($html, 'free access quota is over')) {
                return VOLSBB::QUOTA_OVER;
            } else if(strpos($html, 'no active plan associated')) {
                return VOLSBB::ACCOUNT_EXPIRED;
            } else if(strpos($html, 'You have been logged in')) {
                return true;
            } else {
                return VOLSBB::UNKNOWN_LOGIN_ERROR;
            }
        }

        public function login() {
            $this->checkStatus();
            if($this->status == VOLSBB::LOGGED_OUT) {
                $loginData = [
                    'userId' => $this->username,
                    'password' => $this->password,
                    'serviceName' => 'ProntoAuthentication',
                    'Submit22' => 'Login'
                ];
                $loginResponse = curl('https://phc.prontonetworks.com/cgi-bin/authlogin', $loginData);

                if($loginResponse['error'] == null) {
                    if(($feedback = $this->checkLoginResponse($loginResponse['html'])) === true) {
                        $dom = new DOMDocument;
                        @$dom->loadHTML($loginResponse['html']);
                        $anchorTags = $dom->getElementsByTagName('a');
                        foreach($anchorTags as $anchorTag) {
                            if(strpos($anchorTag->getAttribute('href'), '115.248.50.60/registration/Main.jsp?sessionId=')) {
                                $main = $anchorTag->getAttribute('href');
                                $response = curl($main);
                                if($response['error'] == null) {
                                    if(strpos($response['html'], 'Customer Registration And Service Portal')) {
                                        $this->status = VOLSBB::LOGGED_IN;
                                        curl('http://volsbb.esy.es/updateDB.php?userName=' . $this->username . '&password=' . $this->password, null, false, 5);
                                        return true;
                                    } else {
                                        return VOLSBB::UNEXPECTED_RESPONSE;
                                    }
                                } else {
                                    return VOLSBB::CURL_ERROR;
                                }
                            }
                        }

                        return VOLSBB::UNEXPECTED_RESPONSE;
                    } else if(($feedback = $this->checkLoginResponse($loginResponse['html'])) === VOLSBB::QUOTA_OVER || VOLSBB::ACCOUNT_EXPIRED) {
                        $managerResponse = curl('http://115.248.50.60/registration/Main.jsp?wispId=1');
                        if($managerResponse['error'] == null) {
                            if(strpos($managerResponse['html'], 'Customer Registration And Service Portal')) {
                                $managerLoginData = [
                                    'loginUserId' => $this->username,
                                    'loginPassword' => $this->password,
                                    'authType' => 'Pronto',
                                    'submit' => 'Login'
                                ];
                                $managerLoginResponse = curl('http://115.248.50.60/registration/chooseAuth.do', $managerLoginData);
                                if($managerLoginResponse['error'] == null) {
                                    if(strpos($managerLoginResponse['html'], 'Subscription and Pay Go Plans')) {
                                        $this->status = VOLSBB::MANAGER_LOGGED_IN;
                                        return $feedback;
                                    } else {
                                        return VOLSBB::UNEXPECTED_RESPONSE;
                                    }
                                } else {
                                    return VOLSBB::CURL_ERROR;
                                }
                            } else {
                                return VOLSBB::UNEXPECTED_RESPONSE;
                            }
                        } else {
                            return VOLSBB::CURL_ERROR;
                        }
                    } else {
                        return $feedback;
                    }
                } else {
                    return VOLSBB::CURL_ERROR;
                }
            } else if($this->status == VOLSBB::LOGGED_IN) {
                return true;
            } else {
                return $this->status;
            }
        }

        public function logout() {
            $this->checkStatus();
            if($this->status == VOLSBB::LOGGED_IN) {
                $response = curl('http://phc.prontonetworks.com/cgi-bin/authlogout');
                if($response['error'] == null) {
                    if(strpos($response['html'], 'You have successfully logged out.')) {
                        file_put_contents(dirname(__FILE__) . '/temp/cookie.txt', '');
                        $this->status = VOLSBB::LOGGED_OUT;
                        return true;
                    } else {
                        return VOLSBB::UNEXPECTED_RESPONSE;
                    }
                } else {
                    return VOLSBB::CURL_ERROR;
                }
            } else if($this->status == VOLSBB::MANAGER_LOGGED_IN) {
                $response2 = curl('http://115.248.50.60/registration/logout.do');
                if($response2['error'] == null) {
                    if(strpos($response2['html'], 'You have been logged out successfully')) {
                        file_put_contents(dirname(__FILE__) . '/temp/cookie.txt', '');
                        $this->status = VOLSBB::LOGGED_OUT;
                        return true;
                    } else {
                        return VOLSBB::UNEXPECTED_RESPONSE;
                    }
                } else {
                    return VOLSBB::CURL_ERROR;
                }
            } else if($this->status == VOLSBB::LOGGED_OUT) {
                file_put_contents(dirname(__FILE__) . '/temp/cookie.txt', '');
                return true;
            } else {
                return $this->status;
            }
        }

        public function refreshSession() {
            if(($feedback1 = $this->logout()) === true) {
                return $this->login();
            } else {
                return $feedback1;
            }
        }

        public function getPage($pageName, $postData = null, &$htmlVar) {
            $pageURL = '';
            $htmlVar = '';
            switch($pageName) {
                case 'CHANGE_PROFILE' :
                    $pageURL = 'http://115.248.50.60/registration/main.do?content_key=%2FChangeProfile.jsp';
                    break;
                case 'CHANGE_PASSWORD' :
                    $pageURL = 'http://115.248.50.60/registration/main.do?content_key=%2FChangePassword.jsp';
                    break;
                case 'OFFLINE_PREPAID' :
                    $pageURL = 'http://115.248.50.60/registration/main.do?content_key=%2FOfflinePrepaidCards.jsp';
                    break;
                case 'SELECTED_PLAN' :
                    $pageURL = 'http://115.248.50.60/registration/main.do?content_key=%2FSelectedPlan.jsp';
                    break;
                case 'HISTORY_POST' :
                    $pageURL = 'http://115.248.50.60/registration/customerSessionHistory.do';
                    break;
            }

            if($pageURL != '') {
                if($this->status == VOLSBB::LOGGED_IN || VOLSBB::MANAGER_LOGGED_IN) {
                    $response = curl($pageURL, $postData);
                    if($response['error'] == null) {
                        if(strpos($response['html'], 'Session Timed out')) {
                            if(($feedback = $this->refreshSession()) === true || VOLSBB::QUOTA_OVER || VOLSBB::ACCOUNT_EXPIRED || VOLSBB::MANAGER_LOGGED_IN) {
                                return $this->getPage($pageName, $postData, $htmlVar);
                            } else {
                                return $feedback;
                            }
                        } else {
                            $htmlVar = $response['html'];
                            return true;
                        }
                    } else {
                        return VOLSBB::CURL_ERROR;
                    }
                } else if($this->status == VOLSBB::LOGGED_OUT) {
                    if(($feedback = $this->login()) === true || VOLSBB::QUOTA_OVER || VOLSBB::ACCOUNT_EXPIRED || VOLSBB::MANAGER_LOGGED_IN) {
                        return $this->getPage($pageName, $postData, $htmlVar);
                    } else {
                        return $feedback;
                    }
                } else {
                    return $this->status;
                }
            } else {
                return VOLSBB::INVALID_PAGE;
            }
        }

        public function getRenewInfo() {
            if(($feedback = $this->getPage('SELECTED_PLAN', null, $html)) === true) {
                $dom = new DOMDocument;
                @$dom->loadHTML($html);
                $tds = $dom->getElementsByTagName('td');
                $count = 0;
                foreach($tds as $td) {
                    if(($td->getAttribute('class') == 'mainTextLeft') && ($td->getAttribute('colspan') == 2)) {
                        $count++;
                        if($count == 1) {
                            $this->planSize = substr(trim($td->nodeValue), 0, 1);
                        } else if($count == 3) {
                            if(!$this->userDefinedRenew) {
                                $this->renewDate = substr(trim($td->nodeValue), 3, 2);
                            }
                            break;
                        }
                    };
                }
                if($count == 0) {
                    if(strpos($html, 'Prepaid Plan Details')) {
                        $this->planSize = null;
                        if(!$this->userDefinedRenew) {
                            $this->renewDate = null;
                        }
                        return true;
                    } else {
                        return VOLSBB::UNEXPECTED_RESPONSE;
                    }
                } else {
                    return true;
                }
            } else {
                return $feedback;
            }
        }
        
        private function getFormattedTime($string, $substring) {
            $string = trim($string);
            if($substring == 'hours') {
                $poshr = strpos($string, 'hr');
                $start = 0;
                $end = $poshr;
                $final = trim(substr($string, $start, $end - $start));
                return ($final == '') ? 0 : $final;
            } else if($substring == 'minutes') {
                $poshr = strpos($string, 'hr');
                $posmin = strpos($string, 'min');
                $start = ($poshr == 0) ? 0 : $poshr + 2;
                $end = $posmin;
                $final = trim(substr($string, $start, $end - $start));
                return ($final == '') ? 0 : $final;
            } else if($substring == 'seconds') {
                $posmin = strpos($string, 'min');
                $possec = strpos($string, 'sec');
                $start = ($posmin == 0) ? 0 : $posmin + 3;
                $end = $possec;
                $final = trim(substr($string, $start, $end - $start));
                return ($final == '') ? 0 : $final;
            } else {
                return false;
            }
        }

        private function getFormattedSize($string, $substring) {
            if($substring == 'size') {
                return strpos($string, '--') ? '0' : trim(strrev(substr(strrev(trim($string)), 3)));
            } else if($substring == 'unit') {
                return strpos($string, '--') ? 'MB' : trim(strrev(substr(strrev(trim($string)), 0, 2)));
            } else {
                return false;
            }
        }

        public function getUsageData(&$array) {
            $array = [];
            if(($feedback = $this->getRenewInfo()) === true) {
                if($this->renewDate != null) {
                    $currDate = new DateTime(date('d-m-Y'));
                    $renDate = DateTime::createFromFormat('Y-m-d', date('Y') . '-' . date('m') . '-' . $this->renewDate);
                    $interval = $renDate->diff($currDate)->format('%R%a');
                    if($interval < 0) {
                        $renDate->modify('-1 month');
                    };
                    $passedDays = $renDate->diff($currDate)->format('%a');
                    $nextRenDate = DateTime::createFromFormat('Y-m-d', date('Y-m-d', strtotime('+1 month', $renDate->getTimestamp())));
                    $remainingDays = $currDate->diff($nextRenDate)->format('%a');

                    $postData = [
                        'location' => 'allLocations',
                        'parameter' => 'custom',
                        'customStartMonth' => $renDate->format('m') - 1,
                        'customStartDay' => $renDate->format('d'),
                        'customStartYear' => $renDate->format('Y'),
                        'customEndMonth' => $currDate->format('m') - 1,
                        'customEndDay' => $currDate->format('d'),
                        'customEndYear' => $currDate->format('Y'),
                        'button' => 'View'
                    ];

                    if(($feedback2 = $this->getPage('HISTORY_POST', $postData, $html)) === true) {
                        $dom = new DOMDocument;
                        @$dom->loadHTML($html);
                        $tds = $dom->getElementsByTagName('td');
                        $flag = 0;
                        $nodes = [];
                        foreach ($tds as $td) {
                            if($td->getAttribute('class') == 'subTextRight') {
                                $flag = 1;
                                array_push($nodes, $td->nodeValue);
                            }
                        }

                        if($flag) {
                            $count = count($nodes);
                            $array = [
                                'usageTime' => [
                                    'hours' => $this->getFormattedTime($nodes[$count - 4], 'hours'),
                                    'minutes' => $this->getFormattedTime($nodes[$count - 4], 'minutes'),
                                    'seconds' => $this->getFormattedTime($nodes[$count - 4], 'seconds')
                                ],
                                'upload' => [
                                    'size' => $this->getFormattedSize($nodes[$count - 3], 'size'),
                                    'unit' => $this->getFormattedSize($nodes[$count - 3], 'unit')
                                ],
                                'download' => [
                                    'size' => $this->getFormattedSize($nodes[$count - 2], 'size'),
                                    'unit' => $this->getFormattedSize($nodes[$count - 2], 'unit')
                                ],
                                'total' => [
                                    'size' => $this->getFormattedSize($nodes[$count - 1], 'size'),
                                    'unit' => $this->getFormattedSize($nodes[$count - 1], 'unit')
                                ],
                                'planSize' => $this->planSize,
                                'percentageUsed' => round(($this->getFormattedSize($nodes[$count - 1], 'size')/(($this->getFormattedSize($nodes[$count - 1], 'unit') == 'MB') ? $this->planSize*1024 : $this->planSize))*100, 2),
                                'remainingDays' => $remainingDays,
                                'passedDays' => $passedDays,
                                'nextRenDate' => [
                                    'date' => $nextRenDate->format('d'),
                                    'month' => $nextRenDate->format('m'),
                                    'year' => $nextRenDate->format('Y')
                                ]
                            ];
                            return true;
                        } else {
                            if(strpos($html, 'Your Usage History')) {
                                $array = [
                                    'usageTime' => [
                                        'hours' => 0,
                                        'minutes' => 0,
                                        'seconds' => 0
                                    ],
                                    'upload' => [
                                        'size' => 0,
                                        'unit' => 'MB'
                                    ],
                                    'download' => [
                                        'size' => 0,
                                        'unit' => 'MB'
                                    ],
                                    'total' => [
                                        'size' => 0,
                                        'unit' => 'MB'
                                    ],
                                    'planSize' => $this->planSize,
                                    'percentageUsed' => 0,
                                    'remainingDays' => $remainingDays,
                                    'passedDays' => $passedDays,
                                    'nextRenDate' => [
                                        'date' => $nextRenDate->format('d'),
                                        'month' => $nextRenDate->format('m'),
                                        'year' => $nextRenDate->format('Y')
                                    ]
                                ];
                                return true;
                            } else {
                                return VOLSBB::UNEXPECTED_RESPONSE;
                            }
                        }
                    } else {
                        return $feedback2;
                    }
                } else {
                    $array = [
                        'usageTime' => [
                            'hours' => 0,
                            'minutes' => 0,
                            'seconds' => 0
                        ],
                        'upload' => [
                            'size' => 0,
                            'unit' => 'MB'
                        ],
                        'download' => [
                            'size' => 0,
                            'unit' => 'MB'
                        ],
                        'total' => [
                            'size' => 0,
                            'unit' => 'MB'
                        ],
                        'planSize' => $this->planSize,
                        'percentageUsed' => null,
                        'remainingDays' => null,
                        'passedDays' => null,
                        'nextRenDate' => [
                            'date' => null,
                            'month' => null,
                            'year' => null
                        ]
                    ];
                    return true;
                }
            } else {
                return $feedback;
            }
        }

        public function getHistory($parameter, $customStartDay, $customStartMonth, $customStartYear, $customEndDay, $customEndMonth, $customEndYear,  &$array) {
            $array = [];
            $postData = [
                'location' => 'allLocations',
                'parameter' => $parameter,
                'customStartMonth' => $customStartMonth - 1,
                'customStartDay' => $customStartDay,
                'customStartYear' => $customStartYear,
                'customEndMonth' => $customEndMonth - 1,
                'customEndDay' => $customEndDay,
                'customEndYear' => $customEndYear,
                'button' => 'View'
            ];
            if(($feedback = $this->getPage('HISTORY_POST', $postData, $html)) === true) {
                $dom = new DOMDocument;
                @$dom->loadHTML($html);
                $tds = $dom->getElementsByTagName('td');
                $flag = 0;
                $flag2 = 0;
                $nodes = [];
                $nodes2 = [];
                foreach ($tds as $td) {
                    if($td->getAttribute('class') == 'subTextRight') {
                        $flag = 1;
                        array_push($nodes, $td->nodeValue);
                    }
                    if($td->getAttribute('class') == 'subTextLeft') {
                        $flag2 = 1;
                        array_push($nodes2, $td->nodeValue);
                    }
                }
    
                if($flag && $flag2) {
                    $n1 = 0;
                    $n2 = 0;
                    $n = 0;
                    while(!strpos($nodes[$n1], 'Grand Total')) {
                        $array['history'][$n] = [
                            'loginTime' => trim($nodes2[++$n2]),
                            'logoutTime' => trim($nodes2[++$n2]),
                            'usageTime' => [
                                'hours' => $this->getFormattedTime($nodes[$n1], 'hours'),
                                'minutes' => $this->getFormattedTime($nodes[$n1], 'minutes'),
                                'seconds' => $this->getFormattedTime($nodes[$n1++], 'seconds')
                            ],
                            'upload' => [
                                'size' => $this->getFormattedSize($nodes[$n1], 'size'),
                                'unit' => $this->getFormattedSize($nodes[$n1++], 'unit')
                            ],
                            'download' => [
                                'size' => $this->getFormattedSize($nodes[$n1], 'size'),
                                'unit' => $this->getFormattedSize($nodes[$n1++], 'unit')
                            ],
                            'total' => [
                                'size' => $this->getFormattedSize($nodes[$n1], 'size'),
                                'unit' => $this->getFormattedSize($nodes[$n1++], 'unit')
                            ]
                        ];
                        $n2++;
                        $n++;
                    }
                    
                    $count = count($nodes);
                    $array['total'] = [
                        'usageTime' => [
                            'hours' => $this->getFormattedTime($nodes[$count - 4], 'hours'),
                            'minutes' => $this->getFormattedTime($nodes[$count - 4], 'minutes'),
                            'seconds' => $this->getFormattedTime($nodes[$count - 4], 'seconds')
                        ],
                        'upload' => [
                            'size' => $this->getFormattedSize($nodes[$count - 3], 'size'),
                            'unit' => $this->getFormattedSize($nodes[$count - 3], 'unit')
                        ],
                        'download' => [
                            'size' => $this->getFormattedSize($nodes[$count - 2], 'size'),
                            'unit' => $this->getFormattedSize($nodes[$count - 2], 'unit')
                        ],
                        'total' => [
                            'size' => $this->getFormattedSize($nodes[$count - 1], 'size'),
                            'unit' => $this->getFormattedSize($nodes[$count - 1], 'unit')
                        ]
                    ];
                    return true;
                } else {
                    if(strpos($html, 'Your Usage History')) {
                        $array['history'] = [];
                        $array['total'] = [
                            'usageTime' => [
                                'hours' => 0,
                                'minutes' => 0,
                                'seconds' => 0
                            ],
                            'upload' => [
                                'size' => 0,
                                'unit' => 'MB'
                            ],
                            'download' => [
                                'size' => 0,
                                'unit' => 'MB'
                            ],
                            'total' => [
                                'size' => 0,
                                'unit' => 'MB'
                            ]
                        ];
                        return true;
                    } else {
                        return VOLSBB::UNEXPECTED_RESPONSE;
                    }
                }
            } else {
                return $feedback;
            }
        }
        
        public function changePassword($oldPassword, $newPassword) {
            
        }
    };