# RC-20-21

### FS
O FS cria uma diretoria "FS_files". Aqui vão estar as diretorias dos users cujos nomes correspondem aos respetivos UIDs.<br/>
Nas diretorias dos Users são criados e eliminados (quando necessário) os ficheiros:<br/>
<ul>
<li>"fd.txt" de modo a guardar o file descriptor correspondente ao user</li>
<li>"u.txt" quando é feito um upload, para efeitos de verificação do nome do ficheiro sobre o qual se faz a operação</li>
</ul>

### AS
O AS cria uma diretoria "FILES". Aqui vão estar as diretorias dos users cujos nomes correspondem aos respetivos UIDs.<br/>
Nas diretorias dos Users são criados e eliminados (quando necessário) os ficheiros:<br/>
<ul>
<li>"fd.txt" que guarda o file descriptor correspondente ao user</li>
<li>"password.txt" onde é guardada a password do user</li>
<li>"reg.txt" onde são guardados os dados IP e port do PD associado ao user</li>
<li>"connect.txt" onde são guardados os dados IP e port do user para serem utilizados no verbose mode</li>
<li>"login.txt" que é um ficheiro vazio usado apenas para detetar a existência de login ativo do user</li>
<li>"tid.txt" que existe enquanto o user está num processo ativo de requisição (o mais recente); guarda o FOP, filename (se existir), VC e TID correspondentes</li>
</ul>

### User
O user cria uma diretoria "My_files". Aqui vão estar todos os ficheiros acessíveis pelo mesmo.<br/>
Quando o user realiza um upload do um ficheiro, o path que fornece é relativo à diretoria "My_files".<br/>
Quando efetua um retrive, a localização do ficheiro retornado será também relativa à mesma diretoria. Ou seja, um retrive bem sucedido de um ficheiro "test.txt" resultará na existência de um ficheiro "My_files/test.txt" relativamente à diretoria onde o user está a ser executado.<br/>



