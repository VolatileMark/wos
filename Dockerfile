FROM fedora:36

# Install dependencies
RUN dnf upgrade -y && \
    dnf install -y zsh ncurses curl git sed mesa-dri-drivers sqlite

# Create user
RUN groupadd -g 1000 user && \
    useradd -d /home/user -s /bin/zsh -m user -u 1000 -g 1000 && \
    echo "user:passwd" | chpasswd && \
    echo "user    ALL=(ALL)    NOPASSWD: ALL" >> /etc/sudoers

USER user

# Customize ZSH
RUN export HOME="/home/user" && \
    export ZSH_CUSTOM="$HOME/.oh-my-zsh/custom" && \
    cd $HOME && \
    sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" && \
    git clone --depth=1 https://github.com/romkatv/powerlevel10k.git $ZSH_CUSTOM/themes/powerlevel10k && \
    sed -i "/ZSH_THEME=/c\ZSH_THEME=\"powerlevel10k/powerlevel10k\"" $HOME/.zshrc && \
    echo "POWERLEVEL9K_SHORTEN_STRATEGY=\"truncate_to_last\"" >> $HOME/.zshrc && \
    echo "POWERLEVEL9K_LEFT_PROMPT_ELEMENTS=(user dir vcs status)" >> $HOME/.zshrc && \
    echo "POWERLEVEL9K_RIGHT_PROMPT_ELEMENTS=()" >> $HOME/.zshrc && \
    echo "POWERLEVEL9K_STATUS_OK=false" >> $HOME/.zshrc && \
    echo "POWERLEVEL9K_STATUS_CROSS=true" >> $HOME/.zshrc
