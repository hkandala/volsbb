<?php
    function curl($url, $postdata = NULL, $enableCookies = true, $timeout = 20) {
        $url = str_replace("&amp;", "&", urldecode(trim($url)));
        $cookie = dirname(__FILE__) . '/temp/cookie.txt';
        $curl = curl_init();
        $html = NULL;
        $response = NULL;
        $error = NULL;
        $errorno = NULL;

        curl_setopt($curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows; U; Windows NT 5.1; rv:1.7.3) Gecko/20041001 Firefox/0.10.1");
        curl_setopt($curl, CURLOPT_URL, $url);
        curl_setopt($curl, CURLOPT_ENCODING, "");
        curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($curl, CURLOPT_SAFE_UPLOAD, true);
        curl_setopt($curl, CURLOPT_AUTOREFERER, true);
        curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_setopt($curl, CURLOPT_CONNECTTIMEOUT, $timeout);
        curl_setopt($curl, CURLOPT_TIMEOUT, $timeout);
        curl_setopt($curl, CURLOPT_MAXREDIRS, 10);
        if(isset($postdata) && is_array($postdata)) {
            $postString = '';
            $flag = 0;
            foreach($postdata as $key => $value) {
                $postString .= (($flag == 0) ? '' : '&') . $key . '=' . urlencode($value);
                $flag = 1;
            }
            curl_setopt($curl, CURLOPT_POST, true);
            curl_setopt($curl, CURLOPT_POSTFIELDS, $postString);
        }
        if($enableCookies) {
            curl_setopt($curl, CURLOPT_COOKIEJAR, $cookie);
            curl_setopt($curl, CURLOPT_COOKIEFILE, $cookie);
        }

        $html = curl_exec($curl);
        $response = curl_getinfo($curl);

        if($response['http_code'] == 301 || $response['http_code'] == 302) {
            ini_set("user_agent", "Mozilla/5.0 (Windows; U; Windows NT 5.1; rv:1.7.3) Gecko/20041001 Firefox/0.10.1");
            if($headers = get_headers($response['url'])) {
                foreach($headers as $value) {
                    if (substr(strtolower($value), 0, 9) == "location:") {
                        curl_close($curl);
                        return curl(trim(substr($value, 9, strlen($value))), $postdata);
                    }
                }
            }
        }

        if(!$html) {
            $error = curl_error($curl);
            $errorno = curl_errno($curl);
            curl_close($curl);
            return array("html" => $html, "response" => $response, "error" => $error, "errorno" => $errorno);
        } else {
            curl_close($curl);
            return array("html" => $html, "response" => $response, "error" => $error, "errorno" => $errorno);
        }
    }