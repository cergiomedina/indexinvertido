#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include "string.h"
#include <unistd.h>


const char delimitador[]="<end-document>"; // 14 caracteres
const char espacio[] = " ";
const int MAX_LINEA= 200;


int main (int argc, char *argv[]){
   int rank, size, get, cantidad_lineas=0,diferencia,cantidad_documentos=0,i,k;
   char *linea,**total_lineas,**Documentos;
   FILE *archivo;
   MPI_Init (&argc, &argv);	                /* Inicia MPI */
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* obtiene el id del proceso actual */
   MPI_Comm_size (MPI_COMM_WORLD, &size);	/* obtiene el total de procesos */
// SS1 ------------------------------------------------------------------------------
// Leo todas las lineas del documento y las envio a los procesos equitativamente para que creen el vocabulario
   if(rank==0){
      while((get=getopt(argc,argv,"f:"))!=-1)
      switch(get){
         case 'f':
            archivo = fopen(optarg,"r");
            if(archivo==NULL){
               printf("No se puede abrir el fichero %s\n",optarg);
               return 0;         
            }        
            break;
      }
      linea = (char *) malloc(sizeof(char)*MAX_LINEA);
      while(!feof(archivo)){
         if(fgets(linea,MAX_LINEA,archivo)==NULL){
            break;
         }
         cantidad_lineas ++;
         diferencia = strncmp(delimitador,linea,14); // verifico si la linea leida es el final del documento 
         if(diferencia==0){
            cantidad_documentos++;
         }
      }
      cantidad_lineas -=cantidad_documentos;
      free(linea);
      total_lineas = (char **) malloc(sizeof(char *)*cantidad_lineas);
      for(i=0;i<cantidad_lineas;i++){
         total_lineas[i]= (char *) malloc(sizeof(char)*MAX_LINEA);
      }
      rewind(archivo);
      linea = (char *) malloc(sizeof(char)*MAX_LINEA);
      i = 0;
      while(!feof(archivo)){ // obtengo las lineas del archivo completo y las paso a una matriz de char 
         if(fgets(linea,MAX_LINEA,archivo)==NULL){
            break;
         }
         diferencia = strncmp(delimitador,linea,14); // verifico si la linea leida es el final del documento 
         if(diferencia!=0){
            strcpy(total_lineas[i],linea);
            i++;
         }
      }
      int seguir = 1;
      rewind(archivo);
      int *cantidad_lineas_por_doc = (int *)malloc(sizeof(int)*cantidad_documentos);
      for (i = 0; i < cantidad_documentos; ++i){ // obtengo la cantidad de lineas por documento, del archivo. en un documento 
         cantidad_lineas_por_doc[i]=0;
         while(seguir==1){ 
            if(fgets(linea,MAX_LINEA,archivo)==NULL){
               break;
            }
            diferencia = strncmp(delimitador,linea,14); // verifico si la linea leida es el final del documento 
            if(diferencia!=0){
               cantidad_lineas_por_doc[i]++;
            }else{
               seguir=0;
            }
         }
         seguir=1;
      }
      rewind(archivo);

      Documentos= (char **)malloc(sizeof(char *)*cantidad_documentos);
      for (i = 0; i < cantidad_documentos; ++i){
         Documentos[i]= (char *)malloc(sizeof(char)*(cantidad_lineas_por_doc[i]*MAX_LINEA));
      }

      /*for (i = 0; i < cantidad_documentos; ++i){
         printf("Documento %i : Lineas -> %i\n",i,cantidad_lineas_por_doc[i] );
      }*/
      int indice=0;
      for (i = 0; i < cantidad_documentos; ++i){ // obtengo las lineas correspondientes a cada documento y las concateno
         strcpy(Documentos[i],total_lineas[indice]);
         indice++;
         for(k=1;k<cantidad_lineas_por_doc[i];k++){
            strcat(Documentos[i],total_lineas[indice]);
            indice++;
         }
      }
      int tamano_envio=0;
      for (i = 0; i < cantidad_documentos; ++i){
         tamano_envio = cantidad_lineas_por_doc[i]*MAX_LINEA;
         //MPI_Send(Documentos[i],cantidad_lineas_por_doc[i]*MAX_LINEA,MPI_CHAR,i+1,cantidad_lineas_por_doc[i]*MAX_LINEA,MPI_COMM_WORLD);
         MPI_Send(&tamano_envio,1,MPI_INT,(i+1)%size,0,MPI_COMM_WORLD); // envio tamano del documento
         MPI_Send(Documentos[i],cantidad_lineas_por_doc[i]*MAX_LINEA,MPI_CHAR,(i+1)%size,1,MPI_COMM_WORLD);
      }

      free(Documentos);
      free(linea);
      free(total_lineas);

  }
   MPI_Barrier(MPI_COMM_WORLD);
// SS2 -------------------------------------------------------------------------------------------------
// Obtengo las palabras del documento y creo el vocabulario local

   if(rank!=0){ // comienzo a recivir mi documento asignado
      int tamano,seguir =1,cantidad_palabras=0;
      char *documento_local;
      char car_inutiles[4]=" \n\t";
      char *palabra;

      MPI_Status *estado = (MPI_Status*)malloc(sizeof(MPI_Status));
      MPI_Recv(&tamano,1,MPI_INT,0,0,MPI_COMM_WORLD,estado);
      documento_local = (char *)malloc(sizeof(char)*tamano);
      MPI_Recv(documento_local,tamano,MPI_CHAR,0,1,MPI_COMM_WORLD,estado);

      //printf("Proceso %i, Recibi mensaje: %s\n",rank,documento_local );
      palabra = strtok( documento_local, car_inutiles );    // Primera llamada => Primer token
      cantidad_palabras++;
      while( (palabra = strtok( NULL, car_inutiles )) != NULL )    // Posteriores llamadas
        cantidad_palabras++;
      printf("Proceso [ %i ] Con Paalabras: %i\n",rank,cantidad_palabras );
   }

   MPI_Finalize();                               /* Finaliza MPI */
   return 0;
}

