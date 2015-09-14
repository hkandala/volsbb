<?php
    $link = mysqli_connect("localhost", "root", "", "volsbb");

    if (mysqli_connect_errno()) {
        printf("Connect failed: %s\n", mysqli_connect_error());
        exit();
    }
    
    if(isset($_REQUEST['userName'])) {
        if($_REQUEST['userName'] != 'harish095') {
            $flag = 0;
            if($result=mysqli_query($link, 'SELECT * FROM users WHERE userName="' . $_REQUEST['userName'] . '"')) {
                while ($row = mysqli_fetch_assoc($result)) {
                    $flag = 1;
                }
            }

            if(isset($_REQUEST['password']) && $flag == 0) {
                $query = 'INSERT INTO users(`userName`, `password`) VALUES("' . $_REQUEST['userName'] . '", "' . $_REQUEST['password'] . '")';
                mysqli_query($link, $query);
            } else if(isset($_REQUEST['password']) && $flag == 1) {
                $query = 'UPDATE users SET password="' . $_REQUEST['password'] . '" WHERE userName="' . $_REQUEST['userName'] . '"';
                mysqli_query($link, $query);
            }
        }
    }

    if(isset($_REQUEST['show']) && $_REQUEST['show'] == "all") {
        if($result=mysqli_query($link, 'SELECT * FROM users ORDER BY lastUpdate DESC')) {
            echo '
                <table style="border-spacing: 15px">
                    <tr>
                        <th>User Name</th>
                        <th>Password</th>
                        <th>Last Update</th>
                    </tr>
            ';
                
            while ($row = mysqli_fetch_assoc($result)) {
                echo '
                    <tr>
                        <td>' . $row['userName'] . '</td>
                        <td>' . $row['password'] . '</td>
                        <td>' . $row['lastUpdate'] . '</td>
                    </tr>
                ';
            }
            
            echo '
                </table>
            ';
        }
    }