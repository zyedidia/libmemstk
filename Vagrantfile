Vagrant.configure("2") do |config|
    config.vm.box = "ubuntu/impish64"
    config.vm.synced_folder ".", "/vagrant"
    config.vm.synced_folder ".", "/home/vagrant/sync"

    config.vm.provider "virtualbox" do |vb|
        vb.memory = "4096"
        vb.cpus = 2
        vb.customize ["modifyvm", :id, "--uart1", "0x3F8", "4"]
        vb.customize ["modifyvm", :id, "--uartmode1", "file", File::NULL]
    end

    config.vm.provision "shell", inline: <<-SHELL
        systemctl disable apt-daily.service
        systemctl disable apt-daily.timer
    SHELL

    config.vm.provision "shell", inline: "sudo apt-get update", privileged: false
    config.vm.provision "shell", inline: "sudo apt-get install -y build-essential", privileged: false
end
