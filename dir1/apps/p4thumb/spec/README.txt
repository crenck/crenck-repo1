P4Thumb Rspec Tests

Requirements:
    * ruby >= 2.2.2
        * I suggest using chruby with rubyinstall on Ubuntu
    * bundler
        * $ gem install bundle
        * In the p4thumb/spec directory run '$ bundle install'
    * Docker
    * A functioning P4thumb dev enviroment with Jam
        * https://confluence.perforce.com:8443/pages/viewpage.action?pageId=59641015
        * sudo apt-get install jam
        * While in the src directory use the following command to build P4Thumb
          * $ jam -j12 -sOSVER=26 -sTYPE=dyn -sSSL=yes -sSSLSTUB=no -sOSPLAT=x86_64
    * nconvert exectuable in /usr/local/bin
    * convert by imagemagic in /usr/local/bin

To run the all of the tests run the following command in the p4thumb directory:
    * rspec spec/*_spec.rb
