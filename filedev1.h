/Users/crenck/Perforce/crenck-two-repos/graphdepot1/crenck-repo1

    if (dict.contains("stream"))
    {
        QString streamName = dict["stream"];
        QString action = dict[P4Tag::v_action];
        emit fileUnshelved(streamName, QString(), action);
    }




/*
... repo //graphdepot1/crenck-repo1.git
... repoName //graphdepot1/crenck-repo1
... name refs/heads/dev
... type branch
... sha 329b68358833ae41f35b10192dbbb6139f5e216e

... repo //graphdepot1/crenck-repo1.git
... repoName //graphdepot1/crenck-repo1
... name refs/heads/master
... type branch
... sha e91bab5c3ce826684f86291a97ff71bf8fad41da
... current
*/

 


 Op::TagDict& dataDict = QHash(QString,QString)  key,value



    /*
    for (Op::TagDict::const_iterator it = dataDict.begin(); it != dataDict.end(); it++)
    {
        qDebug() << QString("%1: %2").arg(it.key(), it.value());
    }
    */
