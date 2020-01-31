require 'P4'
require 'fileutils'

RSpec.describe 'P4Thumb' do
  describe '#build' do
    context 'The Jam build tool needs to be invoked to build P4Thumb' do
      it 'will send the system command for jam to build P4Thumb' do
        build_text = @p4thumb.build
        expect(build_text.include?('...updated') ||
               build_text.include?('...found')).to eq true
        expect(File.exist?('../../../p4-bin/bin.linux26x86_64/dyn/p4thumb'))\
          .to eq true
      end
    end
  end

  describe '#start' do
    context 'With P4Thumb now built we need to start it' do
      it 'will call the P4Thumb start with the local config file' do
        @p4thumb.start
        sleep 2 # Wait for P4thumb to possibly fail
        expect(@p4thumb.p4thumb_pid).not_to eq true
      end
    end
  end

  describe '#edit_a_file' do
    context 'With no files on the server' do
      it 'will add a file' do
        p4 = P4.new
        p4.client = 'testClient'
        p4.port = 'localhost:1666'
        p4.user = 'perforce'
        p4.connect

        unless File.exist? 'spec/balloon.jpg'
          FileUtils.cp('spec/test_images/balloon_1.jpg', 'spec/balloon.jpg')
        end
        p4.run_add('spec/balloon.jpg')
        p4.run_submit('-d Adding file')

        sleep(2) # The P4Thumb pull rate is every 1 second
        expect(p4.run_fstat('-Od', 'spec/balloon.jpg') \
               .first['attrDigest-thumb']) \
          .to eq '73C23EDC96F3F60A8C15B32D24ABC976'
      end
    end

    context 'With one file on the server we need a new version' do
      it 'will add a new version of our balloon file' do
        p4 = P4.new
        p4.client = 'testClient'
        p4.port = 'localhost:1666'
        p4.user = 'perforce'
        p4.connect

        p4.run_sync('-f')
        p4.run_edit('spec/balloon.jpg')
        FileUtils.cp('spec/test_images/balloon_2.jpg', 'spec/balloon.jpg')
        p4.run_submit('-d Hello World')

        sleep(2) # The P4Thumb pull rate is every 1 second
        expect(p4.run_fstat('-Od', 'spec/balloon.jpg') \
               .first['attrDigest-thumb']) \
          .to eq 'F4C319190B8D13A7983092D45625FEA3'
      end
    end

    context 'A tiff file now needs added to the server' do
      it 'will add a new tiff file' do
        p4 = P4.new
        p4.client = 'testClient'
        p4.port = 'localhost:1666'
        p4.user = 'perforce'
        p4.connect

        unless File.exist? 'spec/balloon.tiff'
          FileUtils.cp('spec/test_images/balloon_1.tiff', 'spec/balloon.tiff')
        end
        p4.run_add('spec/balloon.tiff')
        p4.run_submit('-d Adding tiff file')

        sleep(2)
        expect(p4.run_fstat('-Od', 'spec/balloon.tiff') \
               .first['attrDigest-thumb']) \
          .to eq 'C405C2A04B503B99F17CE0CF0C40F3D6'
      end
    end
  end

  describe '#stop' do
    context 'The tests have run and now it is time to stop P4Thumb' do
      it 'sends the kill command on the stored PID' do
        expect(@p4thumb.stop).to eq 'P4Thumb is stopped'
      end
    end
  end
end
