<?php
class MySQLHandler {
	//default insecure: no output encoding.
	protected $encodeOutput = FALSE;
	protected $stopSQLInjection = FALSE;
	protected $mSecurityLevel = 0;
	protected $ESAPI = null;
	protected $Encoder = null;
	
	/* Database Configuration */
	/* NOTE: On Samurai, the $dbpass password is "samurai" rather than blank.
	 * If there is any problem connecting, it is almost always one of these
	 * values. */
	protected $mMySQLDatabaseHost = "localhost";
	protected $mMySQLDatabaseUsername = "mutillidae";
	protected $mMySQLDatabasePassword = "mutillidae";
	protected $mMySQLDatabaseName = "mutillidae";

	/* Helper Objects */
	protected $mCustomErrorHandler = null;
	protected $mLogHandler = null;
	
	/* MySQL Object */
	protected $mMySQLConnection = null;

	/* ------------------------------------------
 	 * PRIVATE METHODS
 	 * ------------------------------------------ */
	private function doSetSecurityLevel($pSecurityLevel){
		$this->mSecurityLevel = $pSecurityLevel;
		
		switch ($this->mSecurityLevel){
	   		case "0": // This code is insecure, we are not encoding output
			case "1": // This code is insecure, we are not encoding output
				$this->encodeOutput = FALSE;
				$this->stopSQLInjection = FALSE;
	   		break;
		    		
			case "2":
			case "3":
			case "4":
	   		case "5": // This code is fairly secure
	  			// If we are secure, then we encode all output.
	   			$this->encodeOutput = TRUE;
	   			$this->stopSQLInjection = TRUE;
	   		break;
	   	}// end switch		
	}// end function

	private function doOpenDatabaseConnection(){
		try{	
			$this->mMySQLConnection = new mysqli($this->mMySQLDatabaseHost, $this->mMySQLDatabaseUsername, $this->mMySQLDatabasePassword, $this->mMySQLDatabaseName);
			if (!$this->mMySQLConnection) {
	   		   	throw (new Exception("Error connecting to MySQL database. Connection error: ".$this->mMySQLConnection->connect_errorno." - ".$this->mMySQLConnection->connect_error." Error: ".$this->mMySQLConnection->errorno." - ".$this->mMySQLConnection->error, $this->mMySQLConnection->errorno));
		    }// end if
		} catch (Exception $e) {
			throw(new Exception($this->mCustomErrorHandler->getExceptionMessage("CRITICAL. Error attempting to open MySQL connection. Try checking the connection settings in the MySQLHandler.php class file. If there is a problem connecting, usually one of these settings is incorrect (i.e. - username, password, database name). It is also a good idea to make sure the database is running and that the web site (Mutillidae) is allowed to connect. This error was generated by public function __construct().")));
		}// end try		
	}// end function doOpenDatabaseConnection
			
	/* ------------------------------------------
 	 * CONSTRUCTOR METHOD
 	 * ------------------------------------------ */
	public function __construct($pPathToESAPI, $pSecurityLevel){
		
		$this->doSetSecurityLevel($pSecurityLevel);
		
		//initialize OWASP ESAPI for PHP
		require_once $pPathToESAPI . 'ESAPI.php';
		$this->ESAPI = new ESAPI($pPathToESAPI . 'ESAPI.xml');
		$this->Encoder = $this->ESAPI->getEncoder();
		 
		/* initialize custom error handler */
	    require_once 'CustomErrorHandler.php';
	    $this->mCustomErrorHandler = new CustomErrorHandler($pPathToESAPI, $pSecurityLevel);
	    
		$this->doOpenDatabaseConnection();
	}// end function __construct()

	/* ------------------------------------------
 	 * PUBLIC METHODS
 	 * ------------------------------------------ */
	public function setSecurityLevel($pSecurityLevel){
		$this->doSetSecurityLevel($pSecurityLevel);
	}// end function
	
	public function getSecurityLevel($pSecurityLevel){
		return $this->mSecurityLevel;
	}// end function
	
	public function openDatabaseConnection(){
		$this->doOpenDatabaseConnection();
	}// end function

	public function escapeDangerousCharacters($pString){
		return $this->mMySQLConnection->real_escape_string($pString);
	}//end function

	public function affected_rows(){
		return $this->mMySQLConnection->affected_rows;
	}//end function
		
	public function executeQuery($pQueryString){
		try {
			$lResult = $this->mMySQLConnection->query($pQueryString);
	
			if (!$lResult) {
		    	throw (new Exception("Error executing query: ".$this->mMySQLConnection->error." (".$this->mMySQLConnection->errorno.")"));
		    }// end if there are no results
		    
		    return $lResult;
		} catch (Exception $e) {
			throw(new Exception($this->mCustomErrorHandler->getExceptionMessage($e, "")));
		}// end function

	}// end public function executeQuery

	public function closeDatabaseConnection(){

		try{
			$lResult = $this->mMySQLConnection->close();
			if (!$lResult) {
			   	throw (new Exception("Error executing query. Connection error: ".$this->mMySQLConnection->connect_errorno." - ".$this->mMySQLConnection->connect_error." Error: ".$this->mMySQLConnection->errorno." - ".$this->mMySQLConnection->error, $this->mMySQLConnection->errorno));
			}// end if
		}catch (Exception $e){
			throw(new Exception($this->mCustomErrorHandler->getExceptionMessage($e, "Error attempting to close MySQL connection.")));
		}// end try
		
	}// end public function closeDatabaseConnection

}// end class