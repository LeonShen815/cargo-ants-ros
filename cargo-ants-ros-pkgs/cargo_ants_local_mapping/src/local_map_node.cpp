#include "local_map_node.h"

LocalMapNode::LocalMapNode() :
    nh_(ros::this_node::getName())
{    
    double grid_size_x;
    double grid_size_y;
    double grid_cell_size; 
    double grid_x_max;
    double grid_y_max; 
    int param_x; 
    std::ostringstream topic_name;
    std::string laser_link_name; 
    unsigned int laser_idx_offset; 
    std::ostringstream laser_link_name_full; 
    tf::TransformListener tfl;
    tf::StampedTransform base_2_sensor;
    tf::Matrix3x3 rmat; 

    //get algorithm params from dynamic reconfigure
    nh_.getParam("verbose_mode", this->verbose_);
    nh_.getParam("extract_lines", this->extract_lines_);
    nh_.getParam("mapping_rate", this->rate_);
    nh_.getParam("is_odom_relative", this->is_odom_relative_);
    nh_.getParam("num_lasers", param_x); num_lasers_ = (unsigned int)param_x;
    nh_.getParam("num_radars", param_x); num_radars_ = (unsigned int)param_x;
    nh_.getParam("base_link_name", this->base_link_name_);
    nh_.getParam("laser_idx_offset", param_x); laser_idx_offset = (unsigned int)param_x;
    nh_.getParam("laser_link_name", laser_link_name);
    nh_.getParam("cell_occupancy_odom_th", this->cell_occupancy_odom_th_);
    nh_.getParam("cell_occupancy_cluster_th", this->cell_occupancy_cluster_th_);
    nh_.getParam("cell_occupancy_decay", this->cell_occupancy_decay_);
    nh_.getParam("cell_lidar_hit", this->cell_lidar_hit_);
    if ( verbose_ ) std::cout << "LocalMapNode(): " << __LINE__ <<  std::endl;

    //get grid params from dynamic reconfigure
    nh_.getParam("grid_size_x", grid_size_x);
    nh_.getParam("grid_size_y", grid_size_y);
    nh_.getParam("grid_cell_size", grid_cell_size);
    nh_.getParam("grid_x_max", grid_x_max);
    nh_.getParam("grid_y_max", grid_y_max);
    if ( verbose_ ) std::cout << "LocalMapNode(): " << __LINE__ <<  std::endl;
    
    //allocate the grid_ object. 
    grid_ = new Grid2D(grid_size_x,grid_size_y,grid_cell_size,grid_cell_size,grid_x_max,grid_y_max); 
    if ( verbose_ ) std::cout << "LocalMapNode(): " << __LINE__ <<  std::endl;
    
    //Set fixed sizes of data vectors
    laser_data_.resize(num_lasers_); 
    grid_->laser_points_.resize(num_lasers_);
    if ( verbose_ ) std::cout << "LocalMapNode(): " << __LINE__ <<  std::endl;
    
    //set up odometry subscriber with callback
    odometry_subscriber_ = nh_.subscribe("odometry", 100, &LocalMapNode::odometryCallback, this);
    if ( verbose_ ) std::cout << "LocalMapNode(): " << __LINE__ <<  std::endl;
    
    //set up laser subscribers with callbacks
    laser_subscribers_.resize(num_lasers_); 
    for (unsigned int ii = 0; ii<laser_subscribers_.size(); ii++)
    {
        //set subscribers
        topic_name.str(""); //clears previous content
        topic_name << "laser" << ii;
        laser_subscribers_.at(ii) = nh_.subscribe(topic_name.str(), 1, &LocalMapNode::laserCallback, this);
    }
    
    //set laser_link_id_map_ and laser_mounting_points_ with the homogeneous matrix (assumed fixed)
    laser_mounting_points_.resize(num_lasers_);
    for (unsigned int ii = 0; ii<laser_subscribers_.size(); ii++)
    {
        laser_link_name_full.str("");
        laser_link_name_full << laser_link_name << (ii+laser_idx_offset);
        laser_link_id_map_[laser_link_name_full.str()] = ii; //set mapping between link name and id
        //std::cout << "laser_link_name_full: " << laser_link_name_full.str() << std::endl;
        
        if ( tfl.waitForTransform(base_link_name_, laser_link_name_full.str(), ros::Time(0), ros::Duration(5.)) )
        {
            //look up for transform at TF
            tfl.lookupTransform(base_link_name_, laser_link_name_full.str(), ros::Time(0), base_2_sensor);

            //rotation matrix
            rmat.setRotation(base_2_sensor.getRotation());
            
            //set homogeneous matrix:
            laser_mounting_points_.at(ii) << rmat.getRow(0).x(), rmat.getRow(0).y(), rmat.getRow(0).z(), base_2_sensor.getOrigin().x(),
                                             rmat.getRow(1).x(), rmat.getRow(1).y(), rmat.getRow(1).z(), base_2_sensor.getOrigin().y(),
                                             rmat.getRow(2).x(), rmat.getRow(2).y(), rmat.getRow(2).z(), base_2_sensor.getOrigin().z(),
                                             0,0,0,1;
                                             
            //debug
            if (verbose_)
            {
                std::cout << "H mat from " << base_link_name_ << " to " << laser_link_name_full.str() << ":" << std::endl;
                std::cout << laser_mounting_points_.at(ii) << std::endl; 
            }
        }
        else
        {
            std::cout << "WARNING: Homogeneous Transform from " << base_link_name_ << " to " << laser_link_name_full.str() << " not set. Identity assumed" << std::endl;
            laser_mounting_points_.at(ii) =  Eigen::Matrix4d::Identity();
        }
    }
    if ( verbose_ ) std::cout << "LocalMapNode(): " << __LINE__ <<  std::endl;
    
    //advertise publishers
    grid_publisher_ = nh_.advertise<nav_msgs::OccupancyGrid>("grid_map", 100);
    marker_publisher_ = nh_.advertise<visualization_msgs::MarkerArray>("markers", 100);
	obstacle_map_publisher_ = nh_.advertise<cargo_ants_msgs::ObstacleMap>("obstacle_map", 100);
    if ( verbose_ ) std::cout << "LocalMapNode(): " << __LINE__ <<  std::endl;
    
    //print init info
    std::cout << std::endl << "Local Mapping Running with the following config (II): " << std::endl;
    std::cout << "\tverbose: " << verbose_ << std::endl;
    std::cout << "\textract_lines: " << extract_lines_ << std::endl;
    std::cout << "\trate: " << rate_ << std::endl;
    std::cout << "\tis_odom_relative: " << is_odom_relative_ << std::endl;
    std::cout << "\tnum_lasers: " << num_lasers_ << std::endl;
    std::cout << "\tnum_radars: " << num_radars_ << std::endl;
    std::cout << "\tbase_link_name: " << base_link_name_ << std::endl;
    std::cout << "\tcell_occupancy_odom_th: " << cell_occupancy_odom_th_ << std::endl;
    std::cout << "\tcell_occupancy_cluster_th: " << cell_occupancy_cluster_th_ << std::endl;
    std::cout << "\tcell_occupancy_decay: " << cell_occupancy_decay_ << std::endl;
    std::cout << "\tcell_lidar_hit: " << cell_lidar_hit_ << std::endl;
    
    //print Grid info
    grid_->print(); 
    
    //set last time stamp to now
    ts_past_ = ros::Time::now(); 
    
    //Set laserscanutils hough params 
    hough_params_.range_max_ = 30;
    hough_params_.range_step_ = 0.5;
    hough_params_.theta_step_ = 3*M_PI/180.;
    hough_params_.min_supports_ = 20;    
    line_finder_.setHoughParams(hough_params_);
}

LocalMapNode::~LocalMapNode()
{
    //free memory 
    delete grid_;
}

double LocalMapNode::getRate() const
{
    return rate_;
}

void LocalMapNode::process()
{

    //clear the next grid (1 indicates next)
	grid_->clearGrid(1);
	
	//update the next grid from the current one and applying the motion of all odometry data received since the last iteration. 
    if ( verbose_ ) std::cout << "process(): call to odometryUpdate()" << std::endl;
	grid_->odometryUpdate();
	
	//switch grids: next becomes current
    if ( verbose_ )std::cout << "process(): call to switchGrid()" << std::endl;
    grid_->switchGrid(); 
	
    //update the current grid with lidar hits. 
    if ( verbose_ )std::cout << "process(): call to lidarUpdate()" << std::endl;
    grid_->lidarUpdate();

    //clustering over the current grid
    if ( verbose_ ) std::cout << "process(): call to computeClusters()" << std::endl;
    grid_->computeClusters();
    //if ( verbose_ ) {std::cout << std::endl; grid_->printClusters();}


    //******************* NEW extract lines *****************************
    
    //Clear segment and line lists
    segment_list_.clear(); 
    line_list_.clear();
    
    //run over all laser scan data, and find scan segments
    for ( unsigned int ii = 0; ii < laser_data_.size(); ii++ ) 
    {
        laser_data_[ii].findSegments(segment_list_); 
    }
    
    //run over all segments and compute their parameters. For "big" segments also extract lines 
    for ( std::list<laserscanutils::ScanSegment>::iterator s_it = segment_list_.begin(); s_it != segment_list_.end(); s_it++ ) 
    {
        //compute segment paremeters
        s_it->computeAll(); 
        
        //if segment is enough populated, compute also lines
        if ( s_it->numPoints() > hough_params_.min_supports_ )
        {
            line_finder_.findLines(s_it->points_, line_list_);
        }
    }

    if ( verbose_ ) std::cout << std::endl;
}

void LocalMapNode::publish()
{
    //fill grid message header, info and data
    grid_msg_.header.stamp = ros::Time::now(); 
    grid_msg_.header.frame_id = base_link_name_;
    grid_msg_.info.resolution = grid_->getGridStep();
    grid_msg_.info.width = grid_->getNx();
    grid_msg_.info.height = grid_->getNy();
    grid_msg_.info.origin.position.x = grid_->getXmax()-grid_->getSizeX();
    grid_msg_.info.origin.position.y = grid_->getYmax()-grid_->getSizeY();
    grid_msg_.info.origin.position.z = 0.;
    grid_msg_.info.origin.orientation.x = 0.;
    grid_msg_.info.origin.orientation.y = 0.;
    grid_msg_.info.origin.orientation.z = 0.;
    grid_msg_.info.origin.orientation.w = 1.;
    grid_->getGrid(grid_msg_.data);	
	
    //fill marker message
    marker_msg_.markers.clear();
    marker_msg_.markers.resize( 2*segment_list_.size() + line_list_.size() ); //For each segment, centroids and line strip  + extracted lines
    unsigned int jj = 0; //marker array index
    for ( std::list<laserscanutils::ScanSegment>::iterator s_it = segment_list_.begin(); s_it != segment_list_.end(); s_it++ ) 
    {
        //cluster centroids
        marker_msg_.markers[jj].header.frame_id = base_link_name_;
        marker_msg_.markers[jj].header.stamp = ros::Time::now();
        marker_msg_.markers[jj].id = jj;
        marker_msg_.markers[jj].type = visualization_msgs::Marker::CUBE;
        marker_msg_.markers[jj].action = visualization_msgs::Marker::ADD;
        marker_msg_.markers[jj].pose.position.x = s_it->getCentroid()(0); 
        marker_msg_.markers[jj].pose.position.y = s_it->getCentroid()(1); 
        marker_msg_.markers[jj].pose.position.z = 0.1;
        marker_msg_.markers[jj].scale.x = 0.2;
        marker_msg_.markers[jj].scale.y = 0.2;
        marker_msg_.markers[jj].scale.z = 0.2;
        marker_msg_.markers[jj].color.a = 1.;
        marker_msg_.markers[jj].color.r = 0.3;
        marker_msg_.markers[jj].color.g = 0.3;
        marker_msg_.markers[jj].color.b = 1.;
        marker_msg_.markers[jj].lifetime = ros::Duration(0.1); 
        jj++;
        
        //cluster bounding box
        marker_msg_.markers[jj].header.frame_id = base_link_name_;
        marker_msg_.markers[jj].header.stamp = ros::Time::now();
        marker_msg_.markers[jj].id = jj;
        marker_msg_.markers[jj].type = visualization_msgs::Marker::LINE_STRIP;
        marker_msg_.markers[jj].action = visualization_msgs::Marker::ADD;
        marker_msg_.markers[jj].points.resize(5);//5 points required to close the box. First and last points are the same
        for (unsigned int kk=0; kk<5; kk++)
        {
            marker_msg_.markers[jj].points.at(kk).x = s_it->getBBox(kk%4)(0);
            marker_msg_.markers[jj].points.at(kk).y = s_it->getBBox(kk%4)(1);
            marker_msg_.markers[jj].points.at(kk).z = 0.1;            
        }
        marker_msg_.markers[jj].scale.x = 0.1;
        marker_msg_.markers[jj].color.a = 1.;
        if ( (s_it->points_.cols() < 5) || (s_it->getEvalRatio() < 0.1) ) //small and/or linear cluster !!
        {
            marker_msg_.markers[jj].color.r = 0.3;
            marker_msg_.markers[jj].color.g = 0.3;
            marker_msg_.markers[jj].color.b = 1.;
        }
        else
        {
            marker_msg_.markers[jj].color.r = 1.;
            marker_msg_.markers[jj].color.g = 0.3;
            marker_msg_.markers[jj].color.b = 0.3;            
        }
        marker_msg_.markers[jj].lifetime = ros::Duration(0.1);   
        jj++;
        
    }
    //continue filling the MarkerArray with extracted lines 
    for (std::list<laserscanutils::LineSegment>::iterator line_it=line_list_.begin(); line_it!=line_list_.end(); line_it++)
    {
        //extracted lines
        marker_msg_.markers[jj].header.frame_id = base_link_name_;
        marker_msg_.markers[jj].header.stamp = ros::Time::now();
        marker_msg_.markers[jj].id = jj;
        marker_msg_.markers[jj].type = visualization_msgs::Marker::LINE_STRIP;
        marker_msg_.markers[jj].action = visualization_msgs::Marker::ADD;
        marker_msg_.markers[jj].points.resize(2);//Just one line
        marker_msg_.markers[jj].points.at(0).x = line_it->point_first_(0);
        marker_msg_.markers[jj].points.at(0).y = line_it->point_first_(1);
        marker_msg_.markers[jj].points.at(0).z = 1.;
        marker_msg_.markers[jj].points.at(1).x = line_it->point_last_(0);
        marker_msg_.markers[jj].points.at(1).y = line_it->point_last_(1);
        marker_msg_.markers[jj].points.at(1).z = 1.;
        marker_msg_.markers[jj].scale.x = 0.1;
        marker_msg_.markers[jj].color.a = 1.;
        marker_msg_.markers[jj].color.r = 1.;
        marker_msg_.markers[jj].color.g = 1.;
        marker_msg_.markers[jj].color.b = 0.;
        marker_msg_.markers[jj].lifetime = ros::Duration(0.1);  
        jj++;
    }
    
    //fill ObstacleMap message
    obstacle_map_msg_.obstacles.resize(grid_->cluster_set_.size());
	for (unsigned int ii=0; ii<grid_->cluster_set_.size(); ii++ ) //ii clusters
    {
		obstacle_map_msg_.obstacles.at(ii).type_tag = cargo_ants_msgs::Obstacle::UNKNOWN;
		obstacle_map_msg_.obstacles.at(ii).origin.ox = 0; //TBD
		obstacle_map_msg_.obstacles.at(ii).origin.oy = 0; //TBD
		obstacle_map_msg_.obstacles.at(ii).origin.oth = 0; //TBD
		obstacle_map_msg_.obstacles.at(ii).polylines.resize(1); //wouldn't be necessary this vector of polylines for a single obstacle->TODO: change Obstacle.msg
		obstacle_map_msg_.obstacles.at(ii).polylines.at(0).points.resize(5);
		for (unsigned int jj=0; jj<4; jj++)
		{
			obstacle_map_msg_.obstacles.at(ii).polylines.at(0).points.at(jj).px = 
										grid_->cluster_set_.at(ii).bbox_corners_[jj].first;
			obstacle_map_msg_.obstacles.at(ii).polylines.at(0).points.at(jj).py = 
										grid_->cluster_set_.at(ii).bbox_corners_[jj].second;
		}
		//last point of the polyline message is the first one again
		obstacle_map_msg_.obstacles.at(ii).polylines.at(0).points.at(4).px = 
									grid_->cluster_set_.at(ii).bbox_corners_[0].first;
		obstacle_map_msg_.obstacles.at(ii).polylines.at(0).points.at(4).py = 
									grid_->cluster_set_.at(ii).bbox_corners_[0].second;

	}
    
    //publish
    marker_publisher_.publish(marker_msg_);
    grid_publisher_.publish(grid_msg_);
	obstacle_map_publisher_.publish(obstacle_map_msg_);
}

void LocalMapNode::odometryCallback(const nav_msgs::Odometry::ConstPtr& _msg)
{
    //std::cout << "odometryCallback()" << std::endl;

    ros::Duration dt_ros;
    double dt; 
    
	//reset odometry array
	grid_->odometry_.clear(); 
	
    if (is_odom_relative_)
    {
        // CASE RELATIVE ODOM (2D case. TNO-october14 dataset provides data in this way). Set 1s as integ. time
        grid_->odometry_.push_back(Eigen::Vector4d(_msg->pose.pose.position.x, _msg->pose.pose.position.y, _msg->twist.twist.angular.z, 1));
    }
    else
    {
        // CASE TWIST. Preferred case, but "TNO-october14" rosbag does not provide twist ...
        dt_ros = _msg->header.stamp - ts_past_; 
        ts_past_ = _msg->header.stamp;
        dt = ((double)dt_ros.sec + (double)dt_ros.nsec/(double)1e9);
        grid_->odometry_.push_back(Eigen::Vector4d(_msg->twist.twist.linear.x, _msg->twist.twist.linear.y, _msg->twist.twist.angular.z, dt));
    }
}

void LocalMapNode::laserCallback(const sensor_msgs::LaserScan::ConstPtr& _msg)
{
    //std::cout << "laserCallback()" << std::endl;
    
    unsigned int ii = 0; //iterator over raw scan
    unsigned int ii_ok = 0; //counter of correct hits
    double azimuth = _msg->angle_min; //ray azimuth. Init to angle_min value (azimuth of the first ray)
    unsigned int laser_id;
    Eigen::Vector4d point_device, point_base;
    laserscanutils::LaserScanParams scan_params; 
    
    //from message header.frame_id, gets the laser_id through the laser_link_id_map_
    laser_id = laser_link_id_map_[_msg->header.frame_id]; 
    if ( laser_id > laser_data_.size() ) return; //check error case TODO: See what maps return in the "not-found" case

    //SET laser data at this->laser_data_
    laser_data_[laser_id].ranges_raw_.resize(_msg->ranges.size());
    for ( ii = 0; ii<_msg->ranges.size(); ii++ )
    {
        laser_data_[laser_id].ranges_raw_[ii] = _msg->ranges[ii];
    }
    scan_params.angle_min_ = (laserscanutils::ScalarT)_msg->angle_min;
    scan_params.angle_max_ = (laserscanutils::ScalarT)_msg->angle_max;
    scan_params.angle_step_ = (laserscanutils::ScalarT)_msg->angle_increment;
    scan_params.scan_time_ = (laserscanutils::ScalarT)_msg->scan_time;
    scan_params.range_min_ = (laserscanutils::ScalarT)_msg->range_min;
    //scan_params.range_max_ = (laserscanutils::ScalarT)_msg->range_max;
    scan_params.range_max_ = hough_params_.range_max_; //TODO to think !!
    scan_params.range_std_dev_ = 0.01;//TODO: Set it somewhere out of here!
    laser_data_[laser_id].setLaserScanParams(scan_params);
    laser_data_[laser_id].ranges2xy(laser_mounting_points_.at(laser_id));
}
