%modules = ( # path to module name map
    "QtAmazonS3" => "$basedir/src/s3",
);

%moduleheaders = ( # restrict the module headers to those found in relative path
);

%classnames = (
    "qaccount.h" => "QAccount",
    "qservice.h" => "QService",
    "qbucket.h" => "QBucket"
);
%dependencies = (
    "qtbase" => "refs/heads/dev",
);
