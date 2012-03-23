#include <stdio.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct op op_t;

struct op {
  int op_no;
  int button_no;
  op_t *next;
};

void get_next_orange(op_t *o_current_op, op_t **b_next_op, op_t **b_last_op, int *end, int *j);
void get_next_blue(op_t *b_current_op, op_t **o_next_op, op_t **o_last_op, int *end, int *j);
 
int main(int argc, char *argv[])
{
  int T = 0; /* number of test cases */
  int N = 0; /* number of operations */
  
  int j = 0; /* operation no. seen so far */
  int seconds = 0;       /* time taken so far */
  int end = FALSE;       /* are we at end yet? */

  /* orange values */
  op_t o_current_op; /* current operation */
  int o_current_pos; /* current position */
  op_t *o_next_op;   /* next operation */
  op_t *o_last_op;   /* last operation */
  
  /* blue values */
  op_t b_current_op; /* current operation */
  int b_current_pos; /* current position */
  op_t *b_next_op;   /* next operation */
  op_t *b_last_op;   /* last operation */
  
  /* other variables */
  int i = 0;
  char c;
  op_t *temp = NULL;
  int b_dist = 0;
  int o_dist = 0;
  int min_dist = 0;

  scanf("%d", &T); /* get T */
  fflush(stdout);

  /* for all test cases */
  for (i = 0; i < T; i++) {
    /* initialize */
    o_current_pos = 1;
    o_current_op.op_no = 0;
    o_next_op = NULL;
    o_last_op = NULL;
    b_current_pos = 1;
    b_current_op.op_no = 0;
    b_next_op = NULL;
    b_last_op = NULL;
    end = FALSE;
    seconds = 0;

    scanf("%d", &N);
    
    getchar();
    c = getchar();
    /* if first char is O */
    if (c == 'O') {
      o_current_op.op_no++;
      scanf("%d", &(o_current_op.button_no));
      o_current_op.next = NULL;
      
      j = 2;
      get_next_blue(&b_current_op, &o_next_op, &o_last_op, &end, &j);
    } /* -- end if first char is O */
    /* if first char is B */
    else if (c == 'B') {
      b_current_op.op_no++;
      scanf("%d", &(b_current_op.button_no));
      b_current_op.next = NULL;
      
      j = 2;
      get_next_orange(&o_current_op, &b_next_op, &b_last_op, &end, &j);
    } /* -- end if first char is B */
    
    while (TRUE)
      {
	/* check for end */
	if ((o_current_op.op_no == -1) && 
	    (b_current_op.op_no == -1)) {
	  printf("Case #%d: %d\n", i+1, seconds);
	  break;
	}
	    
	/* if orange is at right place at right time */
	if ((o_current_op.button_no == o_current_pos) && 
	    (o_current_op.op_no != -1) &&
	    ((o_current_op.op_no < b_current_op.op_no) ||
	     (b_current_op.op_no == -1))) {
	  /* if no next orange operation */
	  if (o_next_op == NULL && end == FALSE) {
	    get_next_orange(&o_current_op, &b_next_op, &b_last_op, &end, &j);
	  } /* -- end if no next orange operation */
	  /* otherwise, load next operation */
	  else {
	    o_current_op.op_no = o_next_op->op_no;
	    o_current_op.button_no = o_next_op->button_no;
	    o_current_op.next = NULL;
	    temp = o_next_op;
	    o_next_op = o_next_op->next;
	    free(temp);
	  }	
	  /* move blue one over */
	  if (b_current_pos < b_current_op.button_no) b_current_pos++;
	  else if (b_current_pos > b_current_op.button_no) b_current_pos--;
	  /* update time */
	  seconds++;
	} /* -- end if orange is at right place at right time */
	
	/* if blue is at right place at right time */
	else if ((b_current_op.button_no == b_current_pos) && 
		 (b_current_op.op_no != -1) &&
		 ((b_current_op.op_no < o_current_op.op_no) ||
		  (o_current_op.op_no == -1))) {
	  /* if no next blue operation */
	  if (b_next_op == NULL && end == FALSE) {
	    get_next_blue(&b_current_op, &o_next_op, &o_last_op, &end, &j);
	  } /* -- end if no next blue operation */
	  /* otherwise, load next operation */
	  else {
	    b_current_op.op_no = b_next_op->op_no;
	    b_current_op.button_no = b_next_op->button_no;
	    b_current_op.next = NULL;
	    temp = b_next_op;
	    b_next_op = b_next_op->next;
	    free(temp);
	  }
	  /* move orange one over */
	  if (o_current_pos < o_current_op.button_no) o_current_pos++;
	  else if (o_current_pos > o_current_op.button_no) o_current_pos--;
	  /* update time */
	  seconds++;
	} /* -- end if blue is at right place at right time */

	/* otherwise move both as much as possible */
	else {
	  b_dist = (b_current_pos < b_current_op.button_no) 
	    ? b_current_op.button_no - b_current_pos
	    : b_current_pos - b_current_op.button_no;
	  o_dist = (o_current_pos < o_current_op.button_no) 
	    ? o_current_op.button_no - o_current_pos
	    : o_current_pos - o_current_op.button_no;
	  min_dist = b_dist < o_dist ? b_dist : o_dist;
	  
	  b_current_pos = (b_current_pos < b_current_op.button_no)
	    ? b_current_pos + min_dist 
	    : b_current_pos - min_dist;
	  o_current_pos = (o_current_pos < o_current_op.button_no)
	    ? o_current_pos + min_dist 
	    : o_current_pos - min_dist;
	  seconds += min_dist;

	  /* corner case: if one of them is in right place but at wrong time, move other */
	  if (b_dist == 0) {
	    o_current_pos = (o_current_pos < o_current_op.button_no)
	      ? o_current_pos + o_dist 
	      : o_current_pos - o_dist;
	    seconds += o_dist;
	  }
	  else if (o_dist == 0) {
	    b_current_pos = (b_current_pos < b_current_op.button_no)
	      ? b_current_pos + b_dist 
	      : b_current_pos - b_dist;
	    seconds += b_dist;
	  }
	}	
      } /* -- end while */
  } /* -- end for all test cases */ 
  return 1;
} /* -- end main() */
    
void get_next_blue(op_t *b_current_op, op_t **o_next_op, op_t **o_last_op, int *end, int *j)
{
  char c;
  if (*end == TRUE) return;
  while((c = getchar()))
    {
      if (c == ' ') continue;
      if (c != 'O') break;
      if (*o_next_op == NULL) {
	*o_next_op = malloc(sizeof(op_t));
	*o_last_op = *o_next_op;
	(*o_next_op)->op_no = *j;
	*j = (*j) + 1;
	scanf("%d", &((*o_next_op)->button_no));
	(*o_next_op)->next = NULL;
      }
      else {
	(*o_last_op)->next = malloc(sizeof(op_t));
	*o_last_op = (*o_last_op)->next;
	(*o_last_op)->op_no = *j;
	*j = (*j) + 1;
	scanf("%d", &((*o_last_op)->button_no));
	(*o_last_op)->next = NULL;
      }
    }
  if (c == 'B') {
    b_current_op->op_no = *j;
    *j = (*j) + 1;
    scanf("%d", &(b_current_op->button_no));
    b_current_op->next = NULL;
  }
  else if (c == '\n' || c == EOF) {
    b_current_op->op_no = -1;
    if (*o_next_op == NULL) {
	*o_next_op = malloc(sizeof(op_t));
	*o_last_op = *o_next_op;
	(*o_next_op)->op_no = -1;
	(*o_next_op)->next = NULL;
      }
      else {
	(*o_last_op)->next = malloc(sizeof(op_t));
	*o_last_op = (*o_last_op)->next;
	(*o_last_op)->op_no = -1;
	(*o_last_op)->next = NULL;
      }
    *end = TRUE;
  }
}

void get_next_orange(op_t *o_current_op, op_t **b_next_op, op_t **b_last_op, int *end, int *j)
{
  char c;
  if (*end == TRUE) return;
  while((c = getchar()))
    {
      if (c == ' ') continue;
      if (c != 'B') break;
      if (*b_next_op == NULL) {
	*b_next_op = malloc(sizeof(op_t));
	*b_last_op = *b_next_op;
	(*b_next_op)->op_no = *j;
	*j = (*j) + 1;
	scanf("%d", &((*b_next_op)->button_no));
	(*b_next_op)->next = NULL;
      }
      else {
	(*b_last_op)->next = malloc(sizeof(op_t));
	*b_last_op = (*b_last_op)->next;
	(*b_last_op)->op_no = *j;
	*j = (*j) + 1;
	scanf("%d", &((*b_last_op)->button_no));
	(*b_last_op)->next = NULL;
      }
    }
  if (c == 'O') {
    o_current_op->op_no = *j;
    *j = (*j) + 1;
    scanf("%d", &(o_current_op->button_no));
    o_current_op->next = NULL;
  }
  else if (c == '\n' || c == EOF) {
    o_current_op->op_no = -1;
    if (*b_next_op == NULL) {
	*b_next_op = malloc(sizeof(op_t));
	*b_last_op = *b_next_op;
	(*b_next_op)->op_no = -1;
	(*b_next_op)->next = NULL;
      }
      else {
	(*b_last_op)->next = malloc(sizeof(op_t));
	*b_last_op = (*b_last_op)->next;
	(*b_last_op)->op_no = -1;
	(*b_last_op)->next = NULL;
      }
    *end = TRUE;
  }
}
