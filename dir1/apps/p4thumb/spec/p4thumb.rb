# Matthew Birky
# 11/6/2017

# Functions for building, configuring, starting, and stopping P4Thumb
class P4Thumb
  def build
    # System command to run jam for P4Thumb.
    build_p4thumb = 'jam -j12 ' \
                        '-sOSVER=26 -' \
                        'sTYPE=dyn ' \
                        '-sSSL=yes ' \
                        '-sSSLSTUB=no i' \
                        '-sOSPLAT=x86_64 ' \
                        'p4thumb'
    `( cd ../../ && #{build_p4thumb} )`
  end

  def start(thumb_config = 'spec/thumb_configurations/default.json')
    # Run the system command to start the p4thumb process
    spawn("../../../p4-bin/bin.linux26x86_64/dyn/p4thumb \
          -c #{thumb_config} > spec/thumb_log.txt 2>&1 &")
    sleep 2
    p4thumb_pid
  end

  def stop
    # Check for PID to be set and kill PID
    return if p4thumb_pid.nil?
    `kill #{p4thumb_pid}`
    if p4thumb_pid.nil?
      'P4Thumb is stopped'
    else
      'P4Thumb is still running'
    end
  end

  def p4thumb_pid
    grep =  `ps aux | grep p4thumb`
    match = grep.match(/\d{3,}.*json/)
    return nil if match.nil?
    match[0].match(/\d{3,}/)[0].to_i
  end
end
