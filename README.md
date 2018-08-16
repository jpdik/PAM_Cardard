# PAM_Cardard
A module authentication using PAM from linux and a arduino Serial Communication

#
> ## Português

O objetivo deste projeto foi criar um módulo de autenticação diferençial, assim como vemos hoje em dia muito comum o uso de leitores biométricos, reconhecimento fácial, entre outros.

O módulo desenvolvido pretente usar uma autenticação uilizando a tecnologia RFID juntamente com um Arduino. Foi utilizado:

* [Placa Uno R3 + Cabo USB](https://www.filipeflop.com/produto/placa-uno-r3-cabo-usb-para-arduino/) - Para projeto pequeno.
* [Kit Módulo Leitor Rfid Mfrc522 Mifare](https://www.filipeflop.com/produto/kit-modulo-leitor-rfid-mfrc522-mifare/) - Utilizado em comunicação sem contato a uma frequência de 13,56MHz.
* [Jumpers](https://www.filipeflop.com/produto/kit-jumpers-macho-macho-x65-unidades/) - Fios para montagem rápida em uma protoboard para testes.
* [Protoboard](https://www.filipeflop.com/produto/protoboard-830-pontos/) - Para a prototipagem do circuito.
* [Buzzer](https://www.filipeflop.com/produto/buzzer-ativo-5v/) - Para emitir um pequeno som na leitura do Cartão.
* [Resistor 330Ω](https://www.filipeflop.com/produto/resistor-330%CF%89-14w-x20-unidades/) - Para proteger e diminuir a carga no Buzzer.

#
## Montagem
Para entender melhor a montagem de um Leitor de RFID voce pode usar este [link](https://www.filipeflop.com/blog/controle-acesso-leitor-rfid-arduino/), ou buscar uma documentação sobre o módulo.

A montagem Ficou da seguinte forma:

<p align="center">
  <img src="img/example1.jpg?raw=true"/>
</p>

#
## Arduino

Baixe a plataforma [Arduino IDE](https://www.arduino.cc/en/Main/Software), abra o arquivo em `arduino/Leitor_RFID.ino` nela, compile e mande para a placa de arduino usando um cabo USB.

#
## Entendendo o sistema PAM do linux
Todos os serviços que implementam o módulo PAM possuem uma área especial em `/etc/pam.d/`. Cada serviço implementa uma cadeia de módulos, que precisa ser seguida em ordem, o que impede que um módulo seja executado caso o usuário não consiga se authenticar no módulo anterior, se o mesmo tiver restrições. Estes módulos não são somente a autenticação, como toda parte funcional do usuário (terminal, permissões de acesso, etc.). 

Com queremos implementar um novo método de acesso localmente em uma maquina, seja ela virtual ou real, vamos estudar e alterar o módulo de PAM do `login` que é o sistema de autenticação local do linux,

As colunas de cada arquivo no diretório `pam.d` são divididas da seguinte forma:
```
module_interface     control_flag     module_name module_arguments
```

### module_interface
Quatro tipos de interface do módulo PAM estão disponíveis. Cada um deles corresponde a um aspecto diferente do processo de autorização:

* `auth` - Esta interface do módulo autentica o uso. Por exemplo, solicita e verifica a validade de uma senha. Módulos com essa interface também podem definir credenciais, como associações a grupos ou tickets do Kerberos.

* `account` - Esta interface do módulo verifica se o acesso é permitido. Por exemplo, ele verifica se uma conta de usuário expirou ou se um usuário tem permissão para efetuar login em uma determinada hora do dia.

* `password` - Esta interface do módulo é usada para alterar senhas de usuários.

* `session` - Esta interface do módulo configura e gerencia as sessões do usuário. Os módulos com essa interface também podem executar tarefas adicionais necessárias para permitir o acesso, como montar o diretório inicial de um usuário e disponibilizar a caixa de correio do usuário.

### control_flag

Todos os módulos PAM geram um resultado de sucesso ou falha quando chamados. Sinalizadores de controle informam ao PAM o que fazer com o resultado. Os módulos podem ser empilhados em uma ordem específica e os sinalizadores de controle determinam a importância do sucesso ou falha de um determinado módulo para o objetivo geral de autenticar o usuário para o serviço:


* `required` - O resultado do módulo deve ser bem-sucedido para que a autenticação continue. Se o teste falhar nesse ponto, o usuário não será notificado até que os resultados de todos os testes de módulo que fazem referência a essa interface estejam completos.

* `requisite` - O resultado do módulo deve ser bem sucedido para que a autenticação continue. No entanto, se um teste falhar nesse ponto, o usuário será notificado imediatamente com uma mensagem que reflita o primeiro teste de módulo necessário ou obrigatório.

* `suficiente` - O resultado do módulo é ignorado se falhar. No entanto, se o resultado de um módulo sinalizado for bem-sucedido e nenhum módulo anterior sinalizado tiver falhado, nenhum outro resultado será necessário e o usuário será autenticado no serviço.

* `opcional` - O resultado do módulo é ignorado. Um módulo sinalizado como opcional só se torna necessário para uma autenticação bem-sucedida quando nenhum outro módulo faz referência à interface.

* `include` - Ao contrário dos outros controles, isso não está relacionado a como o resultado do módulo é tratado. Esse sinalizador puxa todas as linhas no arquivo de configuração que corresponde ao parâmetro fornecido e as anexa como um argumento para o módulo.

### module_name e module_arguments
O nome do módulo fornece ao PAM o nome do módulo conectável que contém a interface do módulo especificada. Esses módulos podem receber argumentos, que são passados na frente do módulo. Pode ser um arquivo, um banco de dados, entre outros. O nome do diretório é omitido pois isso depende do sistema operacional onde ficam os módulos do pam. Por exemplo,
no Centos 7, esses módulos ficam em `/lib/security` ou `/lib64/security` (dependendo da arquitetura). É neste local que vamos colocar o nosso módulo `pam_cardard.so`.


### Exemplo: Modulo PAM Login
Arquivo `/etc/pam.d/login` :
```
#%PAM-1.0
auth [user_unknown=ignore success=ok ignore=ignore default=bad] pam_securetty.so
auth       substack     system-auth
auth       include      postlogin
account    required     pam_nologin.so
account    include      system-auth
password   include      system-auth
# pam_selinux.so close should be the first session rule
session    required     pam_selinux.so close
session    required     pam_loginuid.so
session    optional     pam_console.so
# pam_selinux.so open should only be followed by sessions to be executed in the user context
session    required     pam_selinux.so open
session    required     pam_namespace.so
session    optional     pam_keyinit.so force revoke
#session    include      system-auth
session    include      postlogin
-session   optional     pam_ck_connector.so
```

#
## Criando Módulo PAM

Para que o módulo PAM possa funcionar corretamente ele precisa implementar as 3 seguinte funções no .so(Objeto compartilhado - O PAM executa estes módulos independente uns dos outros):

*  `pam_sm_setcred` - É usada para estabelecer, manter e excluir as credenciais de um usuário. Ele deve ser chamado para definir as credenciais depois que um usuário tiver sido autenticado e antes que uma sessão seja aberta para o usuário.

* `pam_sm_acct_mgmt` - Essa função executa a tarefa de estabelecer se o usuário tem permissão para obter acesso no momento. Deve ser entendido que o usuário foi previamente validado por um módulo de autenticação.

* `pam_sm_authenticate` - É usada para autenticar o usuário. O usuário é obrigado a fornecer um token de autenticação, dependendo do serviço de autenticação, geralmente essa é uma senha, mas também pode ser uma impressão digital. Esta é a <b>principal</b> função usada.

Funções em C:
```
PAM_EXTERN int pam_sm_setcred( pam_handle_t *pamh, int flags, int argc, const char **argv ) {
	return <return_module>;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	printf("Acesso Autorizado.\n");
	return <return_module>;
}

PAM_EXTERN int pam_sm_authenticate( pam_handle_t *pamh, int flags,int argc, const char **argv ) {
	return <return_module>;
}
```

Todas essa funções podem retornar os seguinte valores em `<return_module>`:

* `PAM_ABORT` - O aplicação deve finalizar imediatamente ao chamar pam_end(3) primeiro.
* `PAM_AUTH_ERR` - O usuário não foi autenticado no módulo.
* `PAM_CRED_INSUFFICIENT` - Por algum motivo, o aplicativo não possui credenciais suficientes para autenticar o usuário.
* `PAM_AUTHINFO_UNVAIL` - The modules were not able to access the authentication information. This might be due to a network or hardware failure etc.
* `PAM_MAXTRIES` - Os módulos não conseguiram acessar as informações de autenticação. Isto pode ser devido a uma falha de rede ou hardware, etc.
* `PAM_SUCCESS` - O usuário foi autenticado com sucesso.
* `PAM_USER_UNKNOWN` - Usuário desconhecido para o serviço de autenticação.

#
## Instalando o módulo pam_cardard

Este módulo criado por mim, varre as portas `/dev/ttyACM` de 1 a 4, tentando encontrar o módulo arduino e realizar uma comunicação Serial com ele. Caso encontre ele fica aguardando uma resposta do leitor RFID, caso contrário, ele cancela o módulo, passando a responsabilidade para o módulo original de autenticação do linux.

### # Compilando o módulo
Caso queira compilar o módulo manualmente.

Compila o .c e cria o objeto:
> ### gcc -fPIC -fno-stack-protector -c src/pam_cardard.c

Gera o módulo usando o objeto:
> ### sudo ld -x --shared -o bin/pam_cardard.so pam_cardard.o

ou automaticamente rodando o script:

> ### ./setup.sh

Logo após gerar o módulo `pam_cardard.so`, o arquivo deve ser inserido juntamente com os outro modulos do pam. No caso do Centos 7, deve ser inserido em:

> `/lib/security` 

ou

> `/lib64/security`

dependendo da arquitetura.

#
## Configurando o módulo pam_cardard

Vamos agora adicionar o módulo na inicialização do `login` :

Editar o arquivo `/etc/pam.d/login` adicionando nas primeiras linhas:

```
auth       sufficient   pam_cardard.so
account    sufficient   pam_cardard.so
```

Exemplo:
```
#%PAM-1.0
auth       sufficient   pam_cardard.so
account    sufficient   pam_cardard.so
auth [user_unknown=ignore success=ok ignore=ignore default=bad] pam_securetty.so
auth       substack     system-auth
auth       include      postlogin
account    required     pam_nologin.so
account    include      system-auth
password   include      system-auth
# pam_selinux.so close should be the first session rule
session    required     pam_selinux.so close
session    required     pam_loginuid.so
session    optional     pam_console.so
# pam_selinux.so open should only be followed by sessions to be executed in the user context
session    required     pam_selinux.so open
session    required     pam_namespace.so
session    optional     pam_keyinit.so force revoke
#session    include      system-auth
session    include      postlogin
-session   optional     pam_ck_connector.so
```

Isso fará com que o sistema utilize primeiramente o módulo `pam_cardard.so` na autenticação, caso ele falhe, irá chamar o módulo original do linux para autenticação e pedir a senha do usuário. Caso sucesso, ele ira autenticar o usuário utilizando o RFID, ignorando os outro módulos de autenticação, mas carregando os módulos essenciais, como console, dados do usuário, etc.

#
## Utilizando o novo sistema de autenticação

Ligue o cabo USB do arquino no computador que deseja utilizar o RFID.

Após isso, basta simplesmente tentar acessar o sistema, que ele já mostrará o módulo em funcionamento:

<p align="center">
  <img src="img/example2.jpg?raw=true"/>
</p>

<p align="center">
  <img src="img/example3.jpg?raw=true"/>
</p>

#
> ## English
Coming soon.

# Créditos

[simple-pam](https://github.com/beatgammit/simple-pam) de [beatgammit](https://github.com/beatgammit) pelo template de PAM para estudo.

[red-hat](https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/4/html/Reference_Guide/ch-pam.html) pela documentação.

[Herlon Camargo](https://github.com/herloncamargo) pelo auxílio com sistemas linux.