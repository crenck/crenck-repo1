RSpec.configure do |config|
  config.before(:all) do
    client_specs = ['spec/p4_specs/thumbnail_client.txt',
                    'spec/p4_specs/test_client.txt']
    client_specs.each do |file_name|
      next unless IO.readlines(file_name)[-1].match(/Root:/).nil?

      File.open(file_name, 'a') do |file|
        file.write("Root:	#{Dir.pwd}/spec")
      end
    end
    system('docker build -t p4thumb spec/.')
    `docker run --name p4thumb -p 1666:1666 -dit p4thumb`
    @p4thumb = P4Thumb.new
  end

  config.after(:all) do
    `docker kill p4thumb`
    `docker rm p4thumb`
    `rm -f spec/balloon.jpg`
    `rm -f spec/balloon.tiff`
  end
end
