// =======================================================================
// Cardard
// A arduino authentication with RFID
//
// Created by João Paulo de Melo
// =======================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

/**
    Set the bandwitch velocity from Serial Communicatipn
**/
int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/**
    Set to pure timed read
**/
void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

/**
    Read the Serial port And returns the code from Arduino via Reference.
**/
int ler(char *codigo){
    // Set the port to search the arduino.
    // The ttyS0 is used to test in VirtualBox.
    char portname[4][13] = { /*"/dev/ttyS0",*/ "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2", "/dev/ttyACM3" };

    // Set size from list above
    int QNT_PORTAS = sizeof (portname) / sizeof (portname[0]);

    // Variable to handle reference from Serial port.
    int fd;
    // Variable to hadle reference to write in Serial Port.
    int wlen;

    int i;
    for(i=0; i < QNT_PORTAS; i++){
        fd = open(portname[i], O_RDWR | O_NOCTTY | O_SYNC);
        if (fd < 0) {
            //printf("Dispositivo nao encontrado em %s: %s\n", portname[i], strerror(errno));
            continue;
        }
        else{
            break;
        }
    }
    if(i==QNT_PORTAS) return -1;

    set_interface_attribs(fd, B9600);
    //set_mincount(fd, 0);

    /* simple output */
    wlen = write(fd, "Hello!\n", 7);
    if (wlen != 7) {
        printf("Error from write: %d, %d\n", wlen, errno);
        return -1;
    }
    tcdrain(fd);    /* delay for output */

    /* simple noncanonical input */
    do {
        // Variable to alocate data received from Arduino Serial.
        unsigned char buf[80];
        // Variable to hadle reference to read from Serial Port.
        int rdlen;

        // Reading the Serial
        rdlen = read(fd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {
            buf[rdlen] = 0;
            //printf("%s", buf);
            // Ignore trash into return variable.
            if(buf != '/')
                strcat(codigo, buf);
            // Finalize the Reading when find the character 'new line'
            if(buf[strlen(buf)-1] == '\n')
                break;
        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        }
        /* repeat read to get full message */
    } while (1);
    return 1;
}


/**
    Functions from module pam
*/
PAM_EXTERN int pam_sm_setcred( pam_handle_t *pamh, int flags, int argc, const char **argv ) {
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	printf("Acesso Autorizado.\n");
	return PAM_SUCCESS;
}

/**
    Function to authenticate the user
*/
PAM_EXTERN int pam_sm_authenticate( pam_handle_t *pamh, int flags,int argc, const char **argv ) {
	int retval;

    // Ask from username in case of error.
	const char* pUsername;
	char card[100];
	int tentativas = 3;
	retval = pam_get_user(pamh, &pUsername, "Login: ");

    // Welcome message from username
	printf("Bem vindo %s\n", pUsername);

    // Try to read the card from Arduido using a certain number of attempts
    while(tentativas > 0){
        printf("Passe o cartao de acesso no leitor...\n");
        if(ler(card) == 1){
            /** Theres a minimal bug into reading the Serial Port Returning a 'pipe' in the first line
                If that happen we keep this character to be more simple, an remove the 'new line' character
                because the prompt login from linux wait's a 'new line' character to continue your execution.
            */
            if(card[0] == '|')
                card[12] = '\0'; // removing '\n' in this position in case with pipe in first line.
            else
                card[11] = '\0'; // removing '\n' in this position in case when pipe do not happen.

            //printf("%s\n", card);

            /** Simple comparation of String key from the card
                Its more interresting using a database local, this is happens in the next version.
            */
        	if (strcmp(card, "|30-22-72-A3") == 0 || strcmp(card, "30-22-72-A3") == 0) {
                return PAM_SUCCESS; //Alow acess if success.
            }
            else{
                //Deny acess if failure and reduce number of attempts.
                printf("Cartão Inválido...\n");
                tentativas--;
            }
        }
        else{
            /**
                If the card reader is not recognized, We pass the responsibility to the next module.
            */
            printf("Não foi possivel acessar o leitor.\n");

            if (retval != PAM_SUCCESS) {
                return retval;
            }

            return PAM_AUTH_ERR;
        }
        // Clear the variable where the key its stored(Login fails).
        card[0] = '\0';
    }
}
