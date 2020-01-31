require 'P4'
require 'fileutils'

RSpec.describe 'Configuration File' do
  after(:each) do
    @p4thumb.stop
  end

  describe '#MaxFileSize' do
    context 'P4Thumb should ignore files larger than maxFileSize' do
      it 'Starts a thumb instance that has maxFileSize set and ' \
        'checks the file is ignored' do
        @p4thumb.start('spec/thumb_configurations/max_size_100.json')

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

        sleep(1) # This is the pull rate for the loaded p4thumb configuration

        expect(IO.readlines('spec/thumblog.out')[-1] \
               .match(/larger then the maxFileSize/).nil?).to eq false
        expect(p4.run_fstat('-Od', 'spec/balloon.jpg') \
               .first['attrDigest-thumb'].nil?).to eq true

        # There was a bug where logging stopped when the
        # 'larger than the maxFilesSize' message was sent.
        # This final expect is here to make sure logging has continued
        sleep(1)
        expect(IO.readlines('spec/thumblog.out')[-1] \
               .match(/larger then the maxFileSize/).nil?).to eq true
      end
    end
  end

  describe '#BadConversionExe' do
    context 'A configuration file that specifies a bad conversion exe will' \
      'output an error' do
      it 'Attempts to start P4Thumb and then checks the error message' do
        @p4thumb.start('spec/thumb_configurations/bad_conversion_exe.json')

        expect(IO.readlines('spec/thumb_log.txt')[0] \
               .match(/The conversion executable cannnot be found./) \
               .nil?).to eq false
      end
    end
  end

  describe '#InvalidJSON' do
    context 'A configuration that is not JSON compliant will' \
      'output an error' do
      it 'Attempts to start P4Thumb and then checks the error message' do
        @p4thumb.start('spec/thumb_configurations/invalid_json.json')

        expect(IO.readlines('spec/thumb_log.txt')[0] \
               .match(/Error: JSON format failure/) \
               .nil?).to eq false
      end
    end
  end

  describe '#NoClient' do
    context 'A configuration file must have a client' do
      it 'Attempt to start p4thumb without a client' do
        @p4thumb.start('spec/thumb_configurations/no_client.json')

        expect(IO.readlines('spec/thumb_log.txt')[0]).to eq \
          "No client is provided. p4thumb requires a client.\n"
      end
    end
  end
end
